from __future__ import annotations

import importlib.util
import json
from pathlib import Path
import sys
from types import SimpleNamespace
from unittest import mock

import pytest


SCRIPT = Path(__file__).parents[1] / "scripts" / "halofpx-l13-primary-retry.py"
SPEC = importlib.util.spec_from_file_location("halofpx_l59_retry", SCRIPT)
assert SPEC and SPEC.loader
retry = importlib.util.module_from_spec(SPEC)
sys.modules[SPEC.name] = retry
SPEC.loader.exec_module(retry)


class Transport:
    def __init__(self, *, transfer_error: bool = False):
        self.transfer_error = transfer_error
        self.calls = []

    def receive_file(self, host, remote, local, *, expected_size, operation):
        self.calls.append(("receive", host, remote, local, expected_size, operation))
        if self.transfer_error:
            raise retry.CanaryError("injected bounded transfer failure")
        local.write_bytes(b"x" * expected_size)
        return SimpleNamespace(returncode=0, stdout="", stderr="")

    def run_stdin(self, host, argv, stdin, *, operation):
        self.calls.append(("stdin", host, argv, len(stdin), operation))
        return SimpleNamespace(returncode=0, stdout="", stderr="")


def configure(monkeypatch, transport: Transport, *, final: bool) -> str:
    canary_unit = "halofpx-l59-canary.service"
    monkeypatch.setattr(retry, "RESPONSE_HARVESTER_AUTHORITY", {
        "worker": {
            "host": retry.NIMO1,
            "path": "/var/tmp/halofpx-l48-source-nimo1/scripts/"
                    "halofpx_rpc_response_harvest.py",
            "sha256": "a" * 64,
            "interpreter": "python3",
            "source": f"{retry.WORKER_ROOT}/rpc-response-worker.jsonl",
            "staging": f"{retry.WORKER_ROOT}/.rpc-response-worker.harvest",
        },
        "client": {
            "host": retry.NIMO2,
            "path": "/var/tmp/halofpx-l48-source-nimo2/scripts/"
                    "halofpx_rpc_response_harvest.py",
            "sha256": "a" * 64,
            "interpreter": "python3",
            "source": f"{retry.REMOTE_EVIDENCE}/rpc-response-client.jsonl",
            "staging": f"{retry.REMOTE_EVIDENCE}/.rpc-response-client.harvest",
        },
    })
    monkeypatch.setattr(retry, "WORKER_SHA", "b" * 64)
    monkeypatch.setattr(retry, "CANARY_SHA", "c" * 64)
    monkeypatch.setattr(retry, "SSH_TRANSPORT", transport)
    monkeypatch.setattr(
        retry, "write_private_json",
        lambda path, value: path.write_text(
            json.dumps(value, sort_keys=True), encoding="utf-8"))
    retry.DISPOSABLE_UNIT_AUTHORITY.clear()
    retry.DISPOSABLE_UNIT_FINAL_AUTHORITY.clear()
    authority = {
        "invocation_id": "d" * 32,
        "pid": 1234,
        "final_properties": {
            "ExecMainCode": "1", "ExecMainStatus": "4", "Result": "exit-code"},
    }
    target = (retry.NIMO2, canary_unit)
    (retry.DISPOSABLE_UNIT_FINAL_AUTHORITY if final
     else retry.DISPOSABLE_UNIT_AUTHORITY)[target] = authority
    return canary_unit


def helper_ssh(metadata_by_host):
    def invoke(host, *argv, **_kwargs):
        if argv[0] == "sha256sum":
            return SimpleNamespace(
                returncode=0, stdout=f"{'a' * 64}  {argv[-1]}\n",
                stderr="")
        if argv[0] == "python3":
            metadata = metadata_by_host[host]
            return SimpleNamespace(
                returncode=0 if metadata["status"] != "error" else 1,
                stdout=json.dumps(metadata), stderr="")
        raise AssertionError(argv)
    return invoke


def test_missing_both_streams_is_durable_explicit_evidence(tmp_path, monkeypatch):
    canary_unit = configure(monkeypatch, Transport(), final=True)
    missing = {"status": "missing", "reason": "source_absent"}
    monkeypatch.setattr(retry, "ssh", helper_ssh({
        retry.NIMO1: missing, retry.NIMO2: missing}))
    with pytest.raises(retry.CanaryError, match="harvest or authentication failed"):
        retry.harvest_response_boundary_evidence(
            tmp_path, worker_unit="worker", worker_invocation="e" * 32,
            worker_pid=4321, canary_unit=canary_unit)
    receipt = json.loads((tmp_path / "rpc-response-harvest.json").read_text())
    assert receipt["streams"]["worker"]["status"] == "missing"
    assert receipt["streams"]["client"]["status"] == "missing"
    assert receipt["verification"]["status"] == "no_stream_present"


def test_copy_failure_retains_present_metadata_and_final_authority(
        tmp_path, monkeypatch):
    transport = Transport(transfer_error=True)
    canary_unit = configure(monkeypatch, transport, final=True)
    present = {
        "status": "present", "reason": "captured", "copyable": True,
        "bytes": 1, "sha256": "2d711642b726b04401627ca9fbac32f5da7e5"
        "fb3b3b045aefeb9a35f5c3f09", "mode": "0600", "owner": retry.CHANNEL_KEY_OWNER,
    }
    missing = {"status": "missing", "reason": "source_absent"}
    monkeypatch.setattr(retry, "ssh", helper_ssh({
        retry.NIMO1: present, retry.NIMO2: missing}))
    with pytest.raises(retry.CanaryError, match="harvest or authentication failed"):
        retry.harvest_response_boundary_evidence(
            tmp_path, worker_unit="worker", worker_invocation="e" * 32,
            worker_pid=4321, canary_unit=canary_unit)
    receipt = json.loads((tmp_path / "rpc-response-harvest.json").read_text())
    assert receipt["streams"]["worker"]["reason"] == "controller_copy"
    assert receipt["canary"]["authority_status"] == "final"
    assert transport.calls[0][-1] == "evidence"


def test_live_writer_authority_refuses_authentication(tmp_path, monkeypatch):
    canary_unit = configure(monkeypatch, Transport(), final=False)
    missing = {"status": "missing", "reason": "source_absent"}
    monkeypatch.setattr(retry, "ssh", helper_ssh({
        retry.NIMO1: missing, retry.NIMO2: missing}))
    with pytest.raises(retry.CanaryError, match="harvest or authentication failed"):
        retry.harvest_response_boundary_evidence(
            tmp_path, worker_unit="worker", worker_invocation="e" * 32,
            worker_pid=4321, canary_unit=canary_unit)
    receipt = json.loads((tmp_path / "rpc-response-harvest.json").read_text())
    assert receipt["canary"]["authority_status"] == "live"
    assert receipt["verification"]["status"] != "authenticated"


def test_canary_is_quiesced_before_harvest_and_worker_cleanup_is_later():
    source = SCRIPT.read_text(encoding="utf-8")
    finally_block = source.index("if (NIMO2, canary_unit) in DISPOSABLE_UNIT_AUTHORITY:")
    stop_canary = source.index("stop_canary(canary_unit)", finally_block)
    harvest = source.index("harvest_response_boundary_evidence(", stop_canary)
    outer_worker_cleanup = source.index("stop_worker(capture_unit)", harvest)
    assert stop_canary < harvest < outer_worker_cleanup


def test_windows_durability_reopens_and_revalidates(tmp_path, monkeypatch):
    path = tmp_path / "record.json"
    monkeypatch.setattr(retry.os, "name", "nt")
    retry.write_private_json(path, {"value": 1})
    assert json.loads(path.read_text()) == {"value": 1}
    payload = path.read_bytes()
    result = retry._durably_reopen_and_validate(
        path, expected_size=len(payload),
        expected_sha256=__import__("hashlib").sha256(payload).hexdigest())
    assert result["mechanism"] == (
        "windows_file_fsync_atomic_noreplace_reopen_revalidate")
    assert result["status"] == "success"


def test_windows_durability_permission_error_is_not_swallowed(tmp_path, monkeypatch):
    path = tmp_path / "record"
    path.write_bytes(b"x")
    monkeypatch.setattr(retry.os, "name", "nt")
    real_open = retry.os.open

    def deny_reopen(target, flags, *args, **kwargs):
        if (
            Path(target) == path
            and flags & 3 == retry.os.O_RDONLY
        ):
            raise PermissionError("injected")
        return real_open(target, flags, *args, **kwargs)

    monkeypatch.setattr(retry.os, "open", deny_reopen)
    with pytest.raises(retry.CanaryError, match="durability validation failed"):
        retry._durably_reopen_and_validate(
            path, expected_size=1,
            expected_sha256=__import__("hashlib").sha256(b"x").hexdigest())


def test_atomic_noreplace_partial_publication_is_explicit(tmp_path, monkeypatch):
    pending = tmp_path / "pending"
    final = tmp_path / "final"
    pending.write_bytes(b"x")
    real_link = retry.os.link

    def link_then_fail(source, destination, **kwargs):
        real_link(source, destination, **kwargs)
        raise OSError("injected post-link failure")

    monkeypatch.setattr(retry.os, "link", link_then_fail)
    with pytest.raises(retry.CanaryError, match="atomic no-replace"):
        retry._publish_local_evidence_noreplace(
            pending, final, expected_size=1,
            expected_sha256=__import__("hashlib").sha256(b"x").hexdigest())
    assert pending.exists()
    assert final.exists()


def test_atomic_noreplace_refuses_collision(tmp_path):
    pending = tmp_path / "pending"
    final = tmp_path / "final"
    pending.write_bytes(b"x")
    final.write_bytes(b"foreign")
    with pytest.raises(retry.CanaryError, match="collision"):
        retry._publish_local_evidence_noreplace(
            pending, final, expected_size=1,
            expected_sha256=__import__("hashlib").sha256(b"x").hexdigest())
    assert pending.read_bytes() == b"x"
    assert final.read_bytes() == b"foreign"
