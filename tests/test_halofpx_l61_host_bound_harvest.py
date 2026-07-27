from __future__ import annotations

import importlib.util
import json
from pathlib import Path
import sys
from types import SimpleNamespace

import pytest


SCRIPT = Path(__file__).parents[1] / "scripts" / "halofpx-l13-primary-retry.py"
SPEC = importlib.util.spec_from_file_location("halofpx_l61_retry", SCRIPT)
assert SPEC and SPEC.loader
retry = importlib.util.module_from_spec(SPEC)
sys.modules[SPEC.name] = retry
SPEC.loader.exec_module(retry)


def authority() -> dict[str, dict[str, str]]:
    return {
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
    }


def test_host_bound_authority_rejects_cross_host_path(tmp_path, monkeypatch):
    value = authority()
    value["worker"]["path"] = value["client"]["path"]
    monkeypatch.setattr(retry, "RESPONSE_HARVESTER_AUTHORITY", value)
    retry.DISPOSABLE_UNIT_FINAL_AUTHORITY[(retry.NIMO2, "canary")] = {
        "invocation_id": "1" * 32, "pid": 1, "final_properties": {}}
    with pytest.raises(retry.CanaryError, match="worker harvester authority mismatch"):
        retry.harvest_response_boundary_evidence(
            tmp_path, worker_unit="worker", worker_invocation="2" * 32,
            worker_pid=2, canary_unit="canary")


def test_client_probe_harvests_verifies_and_cleans(tmp_path, monkeypatch):
    monkeypatch.setattr(retry, "RESPONSE_HARVESTER_AUTHORITY", authority())
    calls: list[tuple[str, ...]] = []
    existing: set[str] = set()

    def ssh(host, *argv, **_kwargs):
        calls.append((host, *argv))
        if argv[:3] == ("test", "!", "-e"):
            return SimpleNamespace(returncode=0 if argv[3] not in existing else 1,
                                   stdout="", stderr="")
        if argv[0] == "env":
            existing.add(
                f"{retry.REMOTE_EVIDENCE}/rpc-response-client-preflight.jsonl")
            return SimpleNamespace(returncode=0, stdout="", stderr="")
        if argv[0] == "sha256sum":
            return SimpleNamespace(returncode=0, stdout=f"{'a' * 64}  {argv[-1]}\n",
                                   stderr="")
        if argv[0] == "python3" and argv[1].endswith(
                "halofpx_rpc_response_harvest.py"):
            existing.add(
                f"{retry.REMOTE_EVIDENCE}/.rpc-response-client-preflight.harvest")
            return SimpleNamespace(
                returncode=0,
                stdout=json.dumps({
                    "status": "present", "bytes": 10, "sha256": "b" * 64}),
                stderr="")
        if argv[0] == "python3":
            return SimpleNamespace(returncode=0, stdout="{}", stderr="")
        if argv[0] == "rm":
            existing.clear()
            return SimpleNamespace(returncode=0, stdout="", stderr="")
        raise AssertionError(argv)

    monkeypatch.setattr(retry, "ssh", ssh)
    monkeypatch.setattr(
        retry, "write_private_json",
        lambda path, value: path.write_text(json.dumps(value), encoding="utf-8"))
    result = retry.run_l61_client_evidence_probe(tmp_path, "c" * 64)
    assert result["status"] == "pass"
    assert not existing
    assert any(call[1] == "env" for call in calls)


def test_client_probe_refuses_wrong_host_path(tmp_path, monkeypatch):
    value = authority()
    value["client"]["path"] = value["worker"]["path"]
    monkeypatch.setattr(retry, "RESPONSE_HARVESTER_AUTHORITY", value)
    with pytest.raises(retry.CanaryError, match="probe authority mismatch"):
        retry.run_l61_client_evidence_probe(tmp_path, "c" * 64)
