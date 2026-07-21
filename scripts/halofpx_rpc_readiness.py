#!/usr/bin/env python3
"""Bounded application-level readiness probe for HaloFPX disposable RPC workers."""

from __future__ import annotations

import argparse
import hashlib
import json
import os
import socket
import stat
import struct
import time
from dataclasses import dataclass
from pathlib import Path


RPC_CMD_HELLO = 14
# ggml-rpc.cpp enum rpc_cmd: DEVICE_COUNT=15, GRAPH_RECOMPUTE=16, CAPS=17.
RPC_CMD_HALOFPX_STATE_CAPS = 17
RPC_PROTOCOL = (4, 0, 1)
STATE_PROTOCOL = (1, 0)
CAPS_MAGIC = b"HFXCAP2\x00"
CAPS_SIZE = 128
COMMAND_MASK = 0x1F
MAX_REQUEST = 1_048_576
MAX_RESPONSE = 256
MAX_COMPONENTS = 4096
MAX_COMPONENT_BYTES = 1 << 30
MAX_OBJECT_BYTES = 64 << 30
STATE_TIMEOUT_MS = 5000
MAX_ATTEMPTS = 256

HELLO_RESPONSE = struct.Struct("<BBBB24s")
CAPS_RESPONSE = struct.Struct("<8sHHIIIIIIIIQQQQ32s20s")
assert HELLO_RESPONSE.size == 28
assert CAPS_RESPONSE.size == CAPS_SIZE


class ReadinessError(RuntimeError):
    """A fail-closed readiness result."""


class _TransientProbeError(ReadinessError):
    pass


class _MismatchProbeError(ReadinessError):
    pass


@dataclass(frozen=True)
class ExpectedCaps:
    logical_rank: int
    world_size: int
    key_generation: int
    channel_binding: bytes

    def validate(self) -> None:
        if self.logical_rank <= 0 or self.world_size <= 0 or self.logical_rank >= self.world_size:
            raise ValueError("invalid expected logical rank/world")
        if self.key_generation <= 0:
            raise ValueError("invalid expected key generation")
        if len(self.channel_binding) != 32 or not any(self.channel_binding):
            raise ValueError("invalid expected channel binding")


def _endpoint(value: str) -> tuple[str, int]:
    host, separator, port_text = value.rpartition(":")
    if not separator or not host or not port_text.isascii() or not port_text.isdecimal():
        raise ValueError("endpoint must be HOST:PORT")
    port = int(port_text)
    if port <= 0 or port > 65535:
        raise ValueError("endpoint port is out of range")
    return host, port


def _recv_exact(sock: socket.socket, size: int) -> bytes:
    output = bytearray()
    while len(output) < size:
        try:
            part = sock.recv(size - len(output))
        except (TimeoutError, socket.timeout) as exc:
            raise _TransientProbeError("response-timeout") from exc
        except (ConnectionError, OSError) as exc:
            raise _TransientProbeError("early-disconnect") from exc
        if not part:
            raise _TransientProbeError("early-disconnect")
        output.extend(part)
    return bytes(output)


def _exchange(endpoint: tuple[str, int], expected: ExpectedCaps, timeout: float) -> dict[str, object]:
    try:
        sock = socket.create_connection(endpoint, timeout=timeout)
    except (TimeoutError, socket.timeout, ConnectionError, OSError) as exc:
        raise _TransientProbeError("connect-failed") from exc
    with sock:
        sock.settimeout(timeout)
        try:
            sock.sendall(struct.pack("<BQ", RPC_CMD_HELLO, 24) + bytes(24))
        except (TimeoutError, socket.timeout, ConnectionError, OSError) as exc:
            raise _TransientProbeError("hello-send-failed") from exc
        hello_size = struct.unpack("<Q", _recv_exact(sock, 8))[0]
        if hello_size != HELLO_RESPONSE.size:
            raise _MismatchProbeError("malformed-hello-size")
        rpc_major, rpc_minor, rpc_patch, padding, connection_caps = HELLO_RESPONSE.unpack(
            _recv_exact(sock, HELLO_RESPONSE.size)
        )
        if (rpc_major, rpc_minor, rpc_patch) != RPC_PROTOCOL or padding != 0:
            raise _MismatchProbeError("wrong-rpc-version")

        try:
            sock.sendall(struct.pack("<BQ", RPC_CMD_HALOFPX_STATE_CAPS, 0))
        except (TimeoutError, socket.timeout, ConnectionError, OSError) as exc:
            raise _TransientProbeError("caps-send-failed") from exc
        caps_size = struct.unpack("<Q", _recv_exact(sock, 8))[0]
        if caps_size != CAPS_SIZE:
            raise _MismatchProbeError("malformed-caps-size")
        fields = CAPS_RESPONSE.unpack(_recv_exact(sock, CAPS_SIZE))
        (
            magic,
            major,
            minor,
            encoded_size,
            command_mask,
            max_request,
            max_response,
            max_components,
            logical_rank,
            world_size,
            reserved_zero,
            max_component_bytes,
            max_object_bytes,
            timeout_ms,
            key_generation,
            channel_binding,
            reserved,
        ) = fields
        exact = (
            magic == CAPS_MAGIC
            and (major, minor) == STATE_PROTOCOL
            and encoded_size == CAPS_SIZE
            and command_mask == COMMAND_MASK
            and max_request == MAX_REQUEST
            and max_response == MAX_RESPONSE
            and max_components == MAX_COMPONENTS
            and logical_rank == expected.logical_rank
            and world_size == expected.world_size
            and reserved_zero == 0
            and max_component_bytes == MAX_COMPONENT_BYTES
            and max_object_bytes == MAX_OBJECT_BYTES
            and timeout_ms == STATE_TIMEOUT_MS
            and key_generation == expected.key_generation
            and channel_binding == expected.channel_binding
            and reserved == bytes(len(reserved))
        )
        if not exact:
            raise _MismatchProbeError("wrong-caps")
        return {
            "rpc_protocol": ".".join(str(value) for value in RPC_PROTOCOL),
            "state_protocol": ".".join(str(value) for value in STATE_PROTOCOL),
            "caps_bytes": CAPS_SIZE,
            "command_mask": command_mask,
            "commands": ["CAPS", "CAPTURE", "STAGE", "COMMIT_APPLY", "ABORT"],
            "max_request": max_request,
            "max_response": max_response,
            "max_components": max_components,
            "max_component_bytes": max_component_bytes,
            "max_object_bytes": max_object_bytes,
            "state_timeout_ms": timeout_ms,
            "logical_rank": logical_rank,
            "world_size": world_size,
            "key_generation": key_generation,
            "channel_binding_sha256": hashlib.sha256(channel_binding).hexdigest(),
            "connection_caps_sha256": hashlib.sha256(connection_caps).hexdigest(),
        }


def _exchange_feature_off(endpoint: tuple[str, int], timeout: float) -> dict[str, object]:
    try:
        sock = socket.create_connection(endpoint, timeout=timeout)
    except (TimeoutError, socket.timeout, ConnectionError, OSError) as exc:
        raise _TransientProbeError("connect-failed") from exc
    with sock:
        sock.settimeout(timeout)
        try:
            sock.sendall(struct.pack("<BQ", RPC_CMD_HELLO, 24) + bytes(24))
        except (TimeoutError, socket.timeout, ConnectionError, OSError) as exc:
            raise _TransientProbeError("hello-send-failed") from exc
        hello_size = struct.unpack("<Q", _recv_exact(sock, 8))[0]
        if hello_size != HELLO_RESPONSE.size:
            raise _MismatchProbeError("malformed-hello-size")
        rpc_major, rpc_minor, rpc_patch, padding, connection_caps = HELLO_RESPONSE.unpack(
            _recv_exact(sock, HELLO_RESPONSE.size)
        )
        if (rpc_major, rpc_minor, rpc_patch) != RPC_PROTOCOL or padding != 0:
            raise _MismatchProbeError("wrong-rpc-version")
        try:
            sock.sendall(struct.pack("<BQ", RPC_CMD_HALOFPX_STATE_CAPS, 0))
            response = sock.recv(1)
        except socket.timeout as exc:
            raise _TransientProbeError("caps-rejection-timeout") from exc
        except (ConnectionError, OSError):
            response = b""
        if response:
            raise _MismatchProbeError("feature-off-worker-returned-caps")
        return {
            "rpc_protocol": ".".join(str(value) for value in RPC_PROTOCOL),
            "connection_caps_sha256": hashlib.sha256(connection_caps).hexdigest(),
            "caps_rejected": True,
            "feature_off_confirmed": True,
            "admitted": False,
        }


def probe_caps(
    endpoint: str,
    expected: ExpectedCaps,
    *,
    timeout_seconds: float = 120.0,
    attempt_timeout_seconds: float = 2.0,
    initial_backoff_seconds: float = 0.1,
    maximum_backoff_seconds: float = 1.0,
) -> dict[str, object]:
    expected.validate()
    parsed_endpoint = _endpoint(endpoint)
    if timeout_seconds <= 0 or attempt_timeout_seconds <= 0:
        raise ValueError("probe timeouts must be positive")
    if initial_backoff_seconds < 0 or maximum_backoff_seconds < initial_backoff_seconds:
        raise ValueError("invalid retry backoff")
    started = time.monotonic()
    deadline = started + timeout_seconds
    attempts = 0
    failures: list[dict[str, object]] = []
    backoff = initial_backoff_seconds
    while attempts < MAX_ATTEMPTS:
        remaining = deadline - time.monotonic()
        if remaining <= 0:
            break
        attempts += 1
        try:
            result = _exchange(parsed_endpoint, expected, min(attempt_timeout_seconds, remaining))
            result.update({
                "endpoint": endpoint,
                "attempts": attempts,
                "elapsed_ms": round((time.monotonic() - started) * 1000, 3),
                "failures": failures[-16:],
                "admitted": True,
            })
            return result
        except _MismatchProbeError as exc:
            raise ReadinessError(f"readiness-mismatch:{exc}") from exc
        except _TransientProbeError as exc:
            failures.append({"attempt": attempts, "reason": str(exc)})
        remaining = deadline - time.monotonic()
        if remaining <= 0:
            break
        sleep_for = min(backoff, remaining)
        if sleep_for > 0:
            time.sleep(sleep_for)
        backoff = min(maximum_backoff_seconds, max(backoff * 2, initial_backoff_seconds))
    reasons = ",".join(str(item["reason"]) for item in failures[-4:]) or "no-attempt"
    raise ReadinessError(f"readiness-timeout:attempts={attempts}:last={reasons}")


def probe_feature_off(
    endpoint: str,
    *,
    timeout_seconds: float = 120.0,
    attempt_timeout_seconds: float = 2.0,
    initial_backoff_seconds: float = 0.1,
    maximum_backoff_seconds: float = 1.0,
) -> dict[str, object]:
    parsed_endpoint = _endpoint(endpoint)
    if timeout_seconds <= 0 or attempt_timeout_seconds <= 0:
        raise ValueError("probe timeouts must be positive")
    if initial_backoff_seconds < 0 or maximum_backoff_seconds < initial_backoff_seconds:
        raise ValueError("invalid retry backoff")
    started = time.monotonic()
    deadline = started + timeout_seconds
    attempts = 0
    failures: list[dict[str, object]] = []
    backoff = initial_backoff_seconds
    while attempts < MAX_ATTEMPTS:
        remaining = deadline - time.monotonic()
        if remaining <= 0:
            break
        attempts += 1
        try:
            result = _exchange_feature_off(parsed_endpoint, min(attempt_timeout_seconds, remaining))
            result.update({
                "endpoint": endpoint,
                "attempts": attempts,
                "elapsed_ms": round((time.monotonic() - started) * 1000, 3),
                "failures": failures[-16:],
            })
            return result
        except _MismatchProbeError as exc:
            raise ReadinessError(f"feature-off-mismatch:{exc}") from exc
        except _TransientProbeError as exc:
            failures.append({"attempt": attempts, "reason": str(exc)})
        remaining = deadline - time.monotonic()
        if remaining <= 0:
            break
        sleep_for = min(backoff, remaining)
        if sleep_for > 0:
            time.sleep(sleep_for)
        backoff = min(maximum_backoff_seconds, max(backoff * 2, initial_backoff_seconds))
    reasons = ",".join(str(item["reason"]) for item in failures[-4:]) or "no-attempt"
    raise ReadinessError(f"feature-off-timeout:attempts={attempts}:last={reasons}")


def _channel_from_key_file(path: Path) -> bytes:
    flags = os.O_RDONLY | getattr(os, "O_CLOEXEC", 0) | getattr(os, "O_NOFOLLOW", 0)
    fd = os.open(path, flags)
    try:
        details = os.fstat(fd)
        if not stat.S_ISREG(details.st_mode) or details.st_size <= 0 or details.st_size > 256:
            raise ValueError("key file is not an exact bounded regular file")
        if hasattr(os, "geteuid") and (
            details.st_uid != os.geteuid() or details.st_mode & 0o077
        ):
            raise ValueError("key file owner or mode is not protected")
        data = bytearray()
        while len(data) <= 256:
            part = os.read(fd, 257 - len(data))
            if not part:
                break
            data.extend(part)
        if len(data) != details.st_size or len(data) > 256:
            raise ValueError("key file changed or exceeded its bound while reading")
    finally:
        os.close(fd)
    lines = data.splitlines()
    if len(lines) != 2 or len(lines[1]) != 64:
        raise ValueError("key file does not contain an exact channel binding")
    try:
        value = bytes.fromhex(lines[1].decode("ascii"))
    except (UnicodeDecodeError, ValueError) as exc:
        raise ValueError("channel binding is not canonical hex") from exc
    if lines[1].decode("ascii") != value.hex():
        raise ValueError("channel binding is not lowercase canonical hex")
    return value


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--endpoint", required=True)
    parser.add_argument("--logical-rank", type=int)
    parser.add_argument("--world-size", type=int)
    parser.add_argument("--key-generation", type=int)
    parser.add_argument("--expected-channel-key-file", type=Path)
    parser.add_argument("--expect-feature-off", action="store_true")
    parser.add_argument("--timeout-seconds", type=float, default=120.0)
    parser.add_argument("--attempt-timeout-seconds", type=float, default=2.0)
    parser.add_argument("--initial-backoff-seconds", type=float, default=0.1)
    parser.add_argument("--maximum-backoff-seconds", type=float, default=1.0)
    args = parser.parse_args()
    try:
        common = {
            "timeout_seconds": args.timeout_seconds,
            "attempt_timeout_seconds": args.attempt_timeout_seconds,
            "initial_backoff_seconds": args.initial_backoff_seconds,
            "maximum_backoff_seconds": args.maximum_backoff_seconds,
        }
        if args.expect_feature_off:
            if any(value is not None for value in (
                args.logical_rank, args.world_size, args.key_generation, args.expected_channel_key_file
            )):
                raise ValueError("feature-off probe does not accept HaloFPX identity options")
            result = probe_feature_off(args.endpoint, **common)
        else:
            if any(value is None for value in (
                args.logical_rank, args.world_size, args.key_generation, args.expected_channel_key_file
            )):
                raise ValueError("HaloFPX CAPS probe requires rank, world, key generation, and channel file")
            result = probe_caps(
                args.endpoint,
                ExpectedCaps(
                    logical_rank=args.logical_rank,
                    world_size=args.world_size,
                    key_generation=args.key_generation,
                    channel_binding=_channel_from_key_file(args.expected_channel_key_file),
                ),
                **common,
            )
    except (OSError, ValueError, ReadinessError) as exc:
        print(f"readiness failed: {exc}")
        return 1
    print(json.dumps(result, sort_keys=True))
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
