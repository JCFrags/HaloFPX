import importlib.util
import socket
import struct
import sys
import threading
import time
import unittest
from pathlib import Path


SCRIPT = Path(__file__).parents[1] / "scripts" / "halofpx_rpc_readiness.py"
SPEC = importlib.util.spec_from_file_location("halofpx_rpc_readiness", SCRIPT)
assert SPEC and SPEC.loader
readiness = importlib.util.module_from_spec(SPEC)
sys.modules[SPEC.name] = readiness
SPEC.loader.exec_module(readiness)

CHANNEL = bytes([0x22]) * 32
EXPECTED = readiness.ExpectedCaps(1, 2, 7, CHANNEL)


def recv_exact(sock, size):
    output = bytearray()
    while len(output) < size:
        part = sock.recv(size - len(output))
        if not part:
            raise EOFError
        output.extend(part)
    return bytes(output)


def hello(sock, *, version=(4, 0, 1)):
    command, size = struct.unpack("<BQ", recv_exact(sock, 9))
    if command != readiness.RPC_CMD_HELLO or size != 24:
        raise ValueError
    recv_exact(sock, 24)
    payload = readiness.HELLO_RESPONSE.pack(*version, 0, bytes(24))
    sock.sendall(struct.pack("<Q", len(payload)) + payload)


def caps_payload(**overrides):
    fields = {
        "magic": readiness.CAPS_MAGIC,
        "major": 1,
        "minor": 0,
        "encoded_size": readiness.CAPS_SIZE,
        "command_mask": readiness.COMMAND_MASK,
        "max_request": readiness.MAX_REQUEST,
        "max_response": readiness.MAX_RESPONSE,
        "max_components": readiness.MAX_COMPONENTS,
        "logical_rank": 1,
        "world_size": 2,
        "reserved_zero": 0,
        "max_component_bytes": readiness.MAX_COMPONENT_BYTES,
        "max_object_bytes": readiness.MAX_OBJECT_BYTES,
        "timeout_ms": readiness.STATE_TIMEOUT_MS,
        "key_generation": 7,
        "channel_binding": CHANNEL,
        "reserved": bytes(20),
    }
    fields.update(overrides)
    return readiness.CAPS_RESPONSE.pack(*fields.values())


class FixtureServer:
    def __init__(self, handler):
        self.handler = handler
        self.socket = socket.socket()
        self.socket.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
        self.socket.bind(("127.0.0.1", 0))
        self.socket.listen()
        self.socket.settimeout(0.05)
        self.endpoint = f"127.0.0.1:{self.socket.getsockname()[1]}"
        self.stopped = threading.Event()
        self.accepted = 0
        self.thread = threading.Thread(target=self.run, daemon=True)
        self.thread.start()

    def run(self):
        while not self.stopped.is_set():
            try:
                client, _ = self.socket.accept()
            except socket.timeout:
                continue
            except OSError:
                return
            self.accepted += 1
            with client:
                client.settimeout(0.5)
                try:
                    self.handler(client, self.accepted)
                except (BrokenPipeError, ConnectionError, EOFError, OSError, ValueError):
                    pass

    def close(self):
        self.stopped.set()
        self.socket.close()
        self.thread.join(timeout=1)


class RpcReadinessTests(unittest.TestCase):
    def run_probe(self, fixture, **overrides):
        values = {
            "timeout_seconds": 1.0,
            "attempt_timeout_seconds": 0.05,
            "initial_backoff_seconds": 0.01,
            "maximum_backoff_seconds": 0.03,
        }
        values.update(overrides)
        return readiness.probe_caps(fixture.endpoint, EXPECTED, **values)

    def test_listener_first_delayed_protocol_waits_for_exact_caps(self):
        ready_at = time.monotonic() + 0.18

        def handler(client, _attempt):
            if time.monotonic() < ready_at:
                time.sleep(0.08)
                return
            hello(client)
            command, size = struct.unpack("<BQ", recv_exact(client, 9))
            self.assertEqual((command, size), (readiness.RPC_CMD_HALOFPX_STATE_CAPS, 0))
            payload = caps_payload()
            client.sendall(struct.pack("<Q", len(payload)) + payload)

        fixture = FixtureServer(handler)
        started = time.monotonic()
        try:
            result = self.run_probe(fixture)
        finally:
            fixture.close()
        self.assertTrue(result["admitted"])
        self.assertGreaterEqual(result["attempts"], 2)
        self.assertGreaterEqual(time.monotonic() - started, 0.17)
        self.assertTrue(result["failures"])

    def test_timeout_fails_closed_on_early_disconnect(self):
        fixture = FixtureServer(lambda client, _attempt: client.close())
        try:
            with self.assertRaisesRegex(readiness.ReadinessError, "readiness-timeout"):
                self.run_probe(fixture, timeout_seconds=0.2)
        finally:
            fixture.close()

    def test_wrong_capability_is_terminal(self):
        def handler(client, _attempt):
            hello(client)
            recv_exact(client, 9)
            payload = caps_payload(world_size=3)
            client.sendall(struct.pack("<Q", len(payload)) + payload)

        fixture = FixtureServer(handler)
        try:
            with self.assertRaisesRegex(readiness.ReadinessError, "readiness-mismatch:wrong-caps"):
                self.run_probe(fixture)
        finally:
            fixture.close()

    def test_feature_off_hello_then_disconnect_never_admits(self):
        def handler(client, _attempt):
            hello(client)
            recv_exact(client, 9)

        fixture = FixtureServer(handler)
        try:
            with self.assertRaisesRegex(readiness.ReadinessError, "readiness-timeout"):
                self.run_probe(fixture, timeout_seconds=0.2)
        finally:
            fixture.close()

    def test_explicit_feature_off_probe_requires_hello_and_caps_rejection(self):
        def handler(client, _attempt):
            hello(client)
            command, size = struct.unpack("<BQ", recv_exact(client, 9))
            self.assertEqual((command, size), (readiness.RPC_CMD_HALOFPX_STATE_CAPS, 0))

        fixture = FixtureServer(handler)
        try:
            result = readiness.probe_feature_off(
                fixture.endpoint,
                timeout_seconds=0.5,
                attempt_timeout_seconds=0.1,
                initial_backoff_seconds=0.01,
                maximum_backoff_seconds=0.03,
            )
        finally:
            fixture.close()
        self.assertFalse(result["admitted"])
        self.assertTrue(result["feature_off_confirmed"])

    def test_malformed_caps_response_is_terminal(self):
        def handler(client, _attempt):
            hello(client)
            recv_exact(client, 9)
            client.sendall(struct.pack("<Q", 64) + bytes(64))

        fixture = FixtureServer(handler)
        try:
            with self.assertRaisesRegex(readiness.ReadinessError, "malformed-caps-size"):
                self.run_probe(fixture)
        finally:
            fixture.close()

    def test_wrong_rpc_version_is_terminal(self):
        fixture = FixtureServer(lambda client, _attempt: hello(client, version=(3, 9, 9)))
        try:
            with self.assertRaisesRegex(readiness.ReadinessError, "wrong-rpc-version"):
                self.run_probe(fixture)
        finally:
            fixture.close()

    def test_wrong_endpoint_protocol_is_terminal(self):
        def handler(client, _attempt):
            recv_exact(client, 33)
            client.sendall(struct.pack("<Q", 1) + b"x")

        fixture = FixtureServer(handler)
        try:
            with self.assertRaisesRegex(readiness.ReadinessError, "malformed-hello-size"):
                self.run_probe(fixture)
        finally:
            fixture.close()


if __name__ == "__main__":
    unittest.main()
