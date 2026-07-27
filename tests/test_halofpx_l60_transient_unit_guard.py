from __future__ import annotations

import importlib.util
from pathlib import Path
import subprocess
import sys
from types import SimpleNamespace

import pytest


SCRIPT = Path(__file__).parents[1] / "scripts" / "halofpx-l13-primary-retry.py"
SPEC = importlib.util.spec_from_file_location("halofpx_l60_retry", SCRIPT)
assert SPEC and SPEC.loader
retry = importlib.util.module_from_spec(SPEC)
sys.modules[SPEC.name] = retry
SPEC.loader.exec_module(retry)


def snapshot(*, load="not-found", active="inactive", sub="dead",
             pid="0", fragment="", cgroup="", cgroup_pids=(), listener=0,
             registration="", show_rc=0, process_rc=0, registration_rc=0,
             listener_rc=0):
    return {
        "show_returncode": show_rc,
        "process_returncode": process_rc,
        "registration_returncode": registration_rc,
        "listener_returncode": listener_rc,
        "properties": {
            "LoadState": load, "ActiveState": active, "SubState": sub,
            "MainPID": pid, "FragmentPath": fragment,
            "UnitFileState": "", "ControlGroup": cgroup,
        },
        "cgroup_pids": list(cgroup_pids),
        "unit_file_registration": registration,
        "listener_port": 50249,
        "listener_pid": listener,
    }


def configure(monkeypatch, tmp_path):
    retry.UNIT_GUARD_SEQUENCE = 0
    retry.UNIT_GUARD_EVIDENCE_ROOT = tmp_path
    retry.UNIT_PREFIX = "halofpx-l48"
    calls = []

    def fake_ssh(host, *argv, **kwargs):
        calls.append((host, list(argv), kwargs))
        return subprocess.CompletedProcess(argv, 0, "", "")

    monkeypatch.setattr(retry, "ssh", fake_ssh)
    return calls


@pytest.mark.parametrize("sub", ["dead", "failed"])
def test_stale_loaded_inactive_reconciles_once(sub, tmp_path, monkeypatch):
    calls = configure(monkeypatch, tmp_path)
    states = iter([
        snapshot(load="loaded", sub=sub, fragment="/run/user/unit",
                 registration="halofpx-l50-device-gate.service transient"),
        snapshot(),
    ])
    monkeypatch.setattr(retry, "_unit_guard_snapshot", lambda *_args: next(states))
    result = retry.ensure_transient_unit_absent(
        "nimo-1", "halofpx-l50-device-gate", port=50249, phase="prelaunch")
    assert result["status"] == "absent"
    assert [call[1][2] for call in calls] == ["stop", "reset-failed"]
    assert result["manager_scope"] == "user"


def test_failed_ownerless_unit_reconciles(tmp_path, monkeypatch):
    configure(monkeypatch, tmp_path)
    states = iter([
        snapshot(load="loaded", active="failed", sub="failed",
                 fragment="/run/user/unit"),
        snapshot(),
    ])
    monkeypatch.setattr(retry, "_unit_guard_snapshot", lambda *_args: next(states))
    result = retry.ensure_transient_unit_absent(
        "nimo-1", "halofpx-l50-device-gate", port=50249, phase="prelaunch")
    assert result["status"] == "absent"


def test_active_or_owned_unit_refuses_without_stop(tmp_path, monkeypatch):
    calls = configure(monkeypatch, tmp_path)
    monkeypatch.setattr(
        retry, "_unit_guard_snapshot",
        lambda *_args: snapshot(
            load="loaded", active="active", sub="running", pid="55",
            cgroup="/unit.service", cgroup_pids=(55,), listener=55))
    with pytest.raises(retry.CanaryError, match="active or still owns"):
        retry.ensure_transient_unit_absent(
            "nimo-1", "halofpx-l50-device-gate", port=50249,
            phase="prelaunch")
    assert calls == []


def test_delayed_unload_waits_for_full_absence(tmp_path, monkeypatch):
    configure(monkeypatch, tmp_path)
    states = iter([
        snapshot(load="loaded", sub="failed"),
        snapshot(load="loaded", sub="dead"),
        snapshot(load="loaded", sub="dead"),
        snapshot(),
    ])
    monkeypatch.setattr(retry, "_unit_guard_snapshot", lambda *_args: next(states))
    monkeypatch.setattr(retry.time, "sleep", lambda _seconds: None)
    result = retry.ensure_transient_unit_absent(
        "nimo-1", "halofpx-l50-device-gate", port=50249, phase="prelaunch")
    assert result["status"] == "absent"


def test_timeout_retains_refusal_evidence(tmp_path, monkeypatch):
    configure(monkeypatch, tmp_path)
    stale = snapshot(load="loaded", sub="failed")
    monkeypatch.setattr(retry, "_unit_guard_snapshot", lambda *_args: stale)
    ticks = iter([0.0, 0.1, 31.0])
    monkeypatch.setattr(retry.time, "monotonic", lambda: next(ticks))
    monkeypatch.setattr(retry.time, "sleep", lambda _seconds: None)
    with pytest.raises(retry.CanaryError, match="timed out"):
        retry.ensure_transient_unit_absent(
            "nimo-1", "halofpx-l50-device-gate", port=50249,
            phase="prelaunch")
    assert '"status": "timeout"' in next(tmp_path.glob("transient-unit-guard-*.json")).read_text()


def test_wrong_manager_scope_cannot_be_selected_and_user_scope_is_source_owned(
        tmp_path, monkeypatch):
    configure(monkeypatch, tmp_path)
    monkeypatch.setattr(retry, "_unit_guard_snapshot", lambda *_args: snapshot())
    with pytest.raises(retry.CanaryError, match="outside the closed manifest"):
        retry.ensure_transient_unit_absent(
            "nimo-1", "foreign.service", port=None, phase="prelaunch")
    source = SCRIPT.read_text(encoding="utf-8")
    seam = source[source.index("def _unit_guard_snapshot"):
                  source.index("def _unit_guard_absent")]
    assert '"systemctl", "--user", "show"' in seam
    assert '"systemctl", "--system"' not in seam


@pytest.mark.parametrize(
    ("field", "value"),
    [("show_rc", 1), ("process_rc", 1), ("registration_rc", 2),
     ("listener_rc", 1)])
def test_snapshot_command_failure_never_admits(
        field, value, tmp_path, monkeypatch):
    calls = configure(monkeypatch, tmp_path)
    monkeypatch.setattr(
        retry, "_unit_guard_snapshot",
        lambda *_args: snapshot(**{field: value}))
    with pytest.raises(retry.CanaryError, match="authority query failed"):
        retry.ensure_transient_unit_absent(
            "nimo-1", "halofpx-l50-device-gate", port=50249,
            phase="prelaunch")
    assert calls == []


def test_list_unit_files_exact_absent_rc_one_is_admitted(tmp_path, monkeypatch):
    configure(monkeypatch, tmp_path)
    monkeypatch.setattr(
        retry, "_unit_guard_snapshot",
        lambda *_args: snapshot(registration_rc=1, registration=""))
    result = retry.ensure_transient_unit_absent(
        "nimo-1", "halofpx-l50-device-gate", port=50249,
        phase="prelaunch")
    assert result["status"] == "absent"


@pytest.mark.parametrize(
    ("host", "unit", "port"),
    [
        ("nimo-2", "halofpx-l50-device-gate", 50249),
        ("nimo-1", "halofpx-l48-worker-capture", None),
        ("nimo-1", "halofpx-l48-worker-capture", 1),
        ("nimo-1", "halofpx-l48-canary-first-chunk", None),
    ])
def test_swapped_host_or_wrong_port_refuses(host, unit, port, tmp_path, monkeypatch):
    configure(monkeypatch, tmp_path)
    with pytest.raises(retry.CanaryError, match="outside the closed manifest"):
        retry.ensure_transient_unit_absent(
            host, unit, port=port, phase="prelaunch")


def test_first_chunk_unit_is_exactly_admitted(tmp_path, monkeypatch):
    configure(monkeypatch, tmp_path)
    monkeypatch.setattr(retry, "_unit_guard_snapshot", lambda *_args: snapshot())
    result = retry.ensure_transient_unit_absent(
        "nimo-2", "halofpx-l48-canary-first-chunk",
        port=None, phase="prelaunch")
    assert result["status"] == "absent"


def test_systemd_run_invokes_guard_before_spawn(monkeypatch):
    order = []
    monkeypatch.setattr(
        retry, "ensure_transient_unit_absent",
        lambda *args, **kwargs: order.append(("guard", args, kwargs)))
    retry.SSH_TRANSPORT = SimpleNamespace(
        run=lambda *_args, **_kwargs: (
            order.append(("spawn", None, None))
            or SimpleNamespace(returncode=0, stdout="", stderr="")))
    retry.ssh(
        "nimo-1", "systemd-run", "--user",
        "--unit=halofpx-l50-device-gate", "--port", "50249")
    assert [entry[0] for entry in order] == ["guard", "spawn"]
    assert order[0][2]["port"] == 50249
