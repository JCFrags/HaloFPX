from __future__ import annotations

import importlib.util
import inspect
import ast
import json
from pathlib import Path
import subprocess
import sys

import pytest


ROOT = Path(__file__).parents[1]


def load(name, relative):
    spec = importlib.util.spec_from_file_location(name, ROOT / relative)
    assert spec and spec.loader
    module = importlib.util.module_from_spec(spec)
    sys.modules[name] = module
    spec.loader.exec_module(module)
    return module


controller = load("l91_controller", "scripts/halofpx-production-transition.py")
child = load("l91_child", "scripts/halofpx-l13-primary-retry.py")
manifest = json.loads(
    (ROOT / "scripts/halofpx-l77-primary-manifest.json").read_text())


EXPECTED = frozenset({
    ("nimo-1", "halofpx-l50-device-gate", 50249),
    ("nimo-1", "halofpx-l48-worker-capture", 50248),
    ("nimo-1", "halofpx-l48-worker-restore", 50248),
    ("nimo-2", "halofpx-l48-canary-capture", None),
    ("nimo-2", "halofpx-l48-canary-restore", None),
})


def authority_environment(monkeypatch):
    prepared = {"sha256": "a" * 64}
    env = controller.child_environment(prepared, manifest)
    monkeypatch.setenv(
        "HALOFPX_DISPOSABLE_UNIT_AUTHORITY",
        env["HALOFPX_DISPOSABLE_UNIT_AUTHORITY"])
    child.UNIT_GUARD_AUTHORITY = None
    child.install_unit_guard_authority()
    return json.loads(env["HALOFPX_DISPOSABLE_UNIT_AUTHORITY"])


def absent_snapshot(*_args):
    return {
        "show_returncode": 0, "process_returncode": 0,
        "registration_returncode": 1, "listener_returncode": 0,
        "properties": {
            "LoadState": "not-found", "ActiveState": "inactive",
            "SubState": "dead", "MainPID": "0", "FragmentPath": "",
            "UnitFileState": "", "ControlGroup": "",
        },
        "cgroup_pids": [], "unit_file_registration": "",
        "listener_port": None, "listener_pid": 0,
    }


def test_manifest_derived_authority_is_exact(monkeypatch):
    encoded = authority_environment(monkeypatch)
    assert child.UNIT_GUARD_AUTHORITY == EXPECTED
    assert len(encoded) == len(EXPECTED)
    assert manifest["child_argv"] == [
        r"C:\Python314\python.exe",
        r"C:\Users\britt\Documents\HaloFPX\scripts\halofpx-l13-primary-retry.py",
        "--evidence-dir", "{evidence_root}/child", "--l77-primary",
        "--authority-key-file", "/var/tmp/halofpx-l48-control.key",
    ]


def test_every_manifest_tuple_is_consumed_by_same_guard(
        monkeypatch, tmp_path):
    authority_environment(monkeypatch)
    child.UNIT_GUARD_EVIDENCE_ROOT = tmp_path
    child.UNIT_GUARD_SEQUENCE = 0
    monkeypatch.setattr(child, "_unit_guard_snapshot", absent_snapshot)
    for host, unit, port in EXPECTED:
        result = child.ensure_transient_unit_absent(
            host, unit + ".service", port=port, phase="prelaunch")
        assert result["status"] == "absent"


@pytest.mark.parametrize(
    ("host", "unit", "port"),
    [
        ("nimo-1", "halofpx-l48-worker-captur", 50248),
        ("nimo-2", "halofpx-l48-canary-capture-extra", None),
        ("nimo-2", "halofpx-l48-worker-capture", 50248),
        ("nimo-1", "halofpx-l48-worker-capture", 50249),
        ("nimo-1", "halofpx-l48-worker-l68-on", 50248),
    ],
)
def test_near_name_wrong_host_port_and_absent_entry_refuse(
        host, unit, port, monkeypatch, tmp_path):
    authority_environment(monkeypatch)
    child.UNIT_GUARD_EVIDENCE_ROOT = tmp_path
    with pytest.raises(child.CanaryError, match="outside the closed manifest"):
        child.ensure_transient_unit_absent(
            host, unit, port=port, phase="prelaunch")


@pytest.mark.parametrize(
    "unit", ["x.service.service", "x/", "", "x\x00y"])
def test_service_normalization_occurs_once_and_refuses_ambiguity(unit):
    with pytest.raises(child.CanaryError, match="malformed|ambiguous"):
        child._canonical_service_unit(unit)


def test_missing_duplicate_and_malformed_environment_refuse(monkeypatch):
    for raw in ("", "{}", "[]", json.dumps([
            {"host": "nimo-1", "unit": "x.service", "port": 1},
            {"host": "nimo-1", "unit": "x", "port": 1},
    ])):
        monkeypatch.setenv("HALOFPX_DISPOSABLE_UNIT_AUTHORITY", raw)
        with pytest.raises(child.CanaryError):
            child.install_unit_guard_authority()


def test_reachable_l77_launches_and_cleanup_are_closed_static_scan():
    source = inspect.getsource(child)
    required_unit_constructions = {
        'unit = "halofpx-l50-device-gate"',
        'capture_unit = f"{UNIT_PREFIX}-worker-capture"',
        'restore_unit = f"{UNIT_PREFIX}-worker-restore"',
        'unit = f"{UNIT_PREFIX}-canary-{unit_label}"',
        'restore_canary_unit = f"{UNIT_PREFIX}-canary-restore"',
    }
    for construction in required_unit_constructions:
        assert construction in source
    # Every remote launch is routed through ssh(), whose sole systemd-run seam
    # resolves and guards the canonical manifest tuple before transport.
    ssh_source = inspect.getsource(child.ssh)
    assert 'structured_argv[0] == "systemd-run"' in ssh_source
    assert "ensure_transient_unit_absent(" in ssh_source
    assert "subprocess.run" not in ssh_source
    assert "allowed = {" not in inspect.getsource(
        child.ensure_transient_unit_absent)
    assert "install_unit_guard_authority()" in inspect.getsource(child.main)


def test_active_ownership_still_refuses(monkeypatch, tmp_path):
    authority_environment(monkeypatch)
    child.UNIT_GUARD_EVIDENCE_ROOT = tmp_path
    child.UNIT_GUARD_SEQUENCE = 0
    active = absent_snapshot()
    active["properties"].update(
        LoadState="loaded", ActiveState="active", SubState="running",
        MainPID="44", FragmentPath="/run/user/1000/systemd/transient/"
        "halofpx-l48-worker-capture.service",
        ControlGroup="/unit")
    active["cgroup_pids"] = [44]
    active["listener_pid"] = 44
    active["registration_returncode"] = 0
    monkeypatch.setattr(child, "_unit_guard_snapshot", lambda *_: active)
    monkeypatch.setattr(
        child, "ssh",
        lambda *args, **kwargs: subprocess.CompletedProcess(args, 0, "", ""))
    with pytest.raises(child.CanaryError, match="active or still owns"):
        child.ensure_transient_unit_absent(
            "nimo-1", "halofpx-l48-worker-capture",
            port=50248, phase="prelaunch")


def test_real_l77_closed_path_rehearsal_is_exact_and_stable(
        monkeypatch, tmp_path):
    authority_environment(monkeypatch)
    child.configure_l77_primary()
    child.UNIT_GUARD_EVIDENCE_ROOT = tmp_path
    first = child.rehearse_l77_unit_guard_authority(tmp_path)
    assert first["planned_set_equals_authority"] is True
    assert first["all_requests_admitted"] is True
    assert {
        (item["host"], item["unit"], item["port"])
        for item in first["planned"]
    } == EXPECTED
    assert len(first["planned"]) == 12
    assert first["authority_sha256"] == child._authority_digest(EXPECTED)
    receipts = [
        json.loads(path.read_text())
        for path in sorted(tmp_path.glob("unit-guard-request-*.json"))
    ]
    assert len(receipts) == 12
    assert [
        (item["host"], item["unit"], item["port"], item["phase"])
        for item in receipts
    ] == [
        (item["host"], item["unit"], item["port"], item["phase"])
        for item in first["planned"]
    ]

    second_root = tmp_path / "second"
    second_root.mkdir()
    second = child.rehearse_l77_unit_guard_authority(second_root)
    assert second["authority_sha256"] == first["authority_sha256"]
    assert second["planned"] == first["planned"]


def test_rehearsal_invokes_real_worker_and_canary_cleanup_seams(
        monkeypatch, tmp_path):
    authority_environment(monkeypatch)
    child.configure_l77_primary()
    child.UNIT_GUARD_EVIDENCE_ROOT = tmp_path
    worker_calls = []
    canary_calls = []
    real_stop_worker = child.stop_worker
    real_stop_canary = child.stop_canary

    def observe_worker(unit, port, **kwargs):
        worker_calls.append((unit, port, kwargs))
        return real_stop_worker(unit, port, **kwargs)

    def observe_canary(unit, **kwargs):
        canary_calls.append((unit, kwargs))
        return real_stop_canary(unit, **kwargs)

    monkeypatch.setattr(child, "stop_worker", observe_worker)
    monkeypatch.setattr(child, "stop_canary", observe_canary)
    child.rehearse_l77_unit_guard_authority(tmp_path)
    assert [(unit, port) for unit, port, _ in worker_calls] == [
        ("halofpx-l50-device-gate", 50249),
        ("halofpx-l48-worker-capture", 50248),
        ("halofpx-l48-worker-capture", 50248),
        ("halofpx-l48-worker-restore", 50248),
        ("halofpx-l50-device-gate", 50249),
    ]
    assert all(call[2] == {"_rehearsal_only": True} for call in worker_calls)
    assert [unit for unit, _ in canary_calls] == [
        "halofpx-l48-canary-restore",
        "halofpx-l48-canary-restore",
    ]
    assert all(call[1] == {"_rehearsal_only": True} for call in canary_calls)


def test_stop_worker_requires_exact_explicit_configured_port(
        monkeypatch, tmp_path):
    authority_environment(monkeypatch)
    child.configure_l77_primary()
    child.UNIT_GUARD_EVIDENCE_ROOT = tmp_path
    child.UNIT_GUARD_REQUEST_SEQUENCE = 0
    with pytest.raises(TypeError):
        child.stop_worker("halofpx-l48-worker-capture")
    with pytest.raises(child.CanaryError, match="outside the closed manifest"):
        child.stop_worker(
            "halofpx-l48-worker-capture", 50184, _rehearsal_only=True)
    child.stop_worker(
        "halofpx-l48-worker-capture", 50248, _rehearsal_only=True)
    child.stop_worker(
        "halofpx-l50-device-gate", 50249, _rehearsal_only=True)
    receipts = [
        json.loads(path.read_text())
        for path in sorted(tmp_path.glob("unit-guard-request-*.json"))
    ]
    assert [(item["port"], item["membership"]) for item in receipts] == [
        (50184, False), (50248, True), (50249, True)]


def test_feature_off_legacy_authority_remains_inert_and_explicit(
        monkeypatch, tmp_path):
    monkeypatch.setattr(child, "UNIT_GUARD_AUTHORITY", None)
    monkeypatch.setattr(child, "UNIT_GUARD_AUTHORITY_SHA256", "")
    monkeypatch.setattr(child, "MANIFEST_UNIT_AUTHORITY_REQUIRED", False)
    monkeypatch.setattr(child, "UNIT_PREFIX", "halofpx-l48")
    monkeypatch.setattr(child, "PORT", 50248)
    child.UNIT_GUARD_EVIDENCE_ROOT = tmp_path
    child.UNIT_GUARD_REQUEST_SEQUENCE = 0
    assert child.worker_cleanup_port(
        "halofpx-l48-worker-capture") == 50248
    child.stop_worker(
        "halofpx-l48-worker-capture", 50248, _rehearsal_only=True)
    receipt = json.loads(
        (tmp_path / "unit-guard-request-001.json").read_text())
    assert receipt["membership"] is True
    assert receipt["port"] == 50248


def test_no_callable_default_captures_a_configuration_global():
    source = (ROOT / "scripts/halofpx-l13-primary-retry.py").read_text()
    tree = ast.parse(source)
    assigned_globals = {
        target.id
        for node in tree.body
        if isinstance(node, (ast.Assign, ast.AnnAssign))
        for target in (
            node.targets if isinstance(node, ast.Assign) else [node.target])
        if isinstance(target, ast.Name)
    }
    captured = []
    for node in ast.walk(tree):
        if not isinstance(node, (ast.FunctionDef, ast.AsyncFunctionDef)):
            continue
        defaults = [*node.args.defaults, *node.args.kw_defaults]
        captured.extend(
            (node.name, default.id)
            for default in defaults
            if isinstance(default, ast.Name) and default.id in assigned_globals)
    assert captured == []


def test_refusal_retains_requested_tuple_and_membership(
        monkeypatch, tmp_path):
    authority_environment(monkeypatch)
    child.UNIT_GUARD_EVIDENCE_ROOT = tmp_path
    child.UNIT_GUARD_REQUEST_SEQUENCE = 0
    with pytest.raises(child.CanaryError, match="outside the closed manifest"):
        child.ensure_transient_unit_absent(
            "nimo-1", "halofpx-l48-worker-restore",
            port=50249, phase="prelaunch")
    receipt = json.loads(
        (tmp_path / "unit-guard-request-001.json").read_text())
    assert receipt == {
        "schema": "halofpx.l92.unit-guard-request.v1",
        "sequence": 1,
        "host": "nimo-1",
        "unit": "halofpx-l48-worker-restore",
        "port": 50249,
        "phase": "prelaunch",
        "authority_sha256": child.UNIT_GUARD_AUTHORITY_SHA256,
        "membership": False,
    }


def test_exact_l77_main_publishes_rehearsal_before_ssh(
        monkeypatch, tmp_path):
    prepared = {"sha256": "a" * 64}
    env = controller.child_environment(prepared, manifest)
    monkeypatch.setenv(
        "HALOFPX_DISPOSABLE_UNIT_AUTHORITY",
        env["HALOFPX_DISPOSABLE_UNIT_AUTHORITY"])
    child.UNIT_GUARD_AUTHORITY = None
    child.UNIT_GUARD_AUTHORITY_SHA256 = ""
    child.UNIT_GUARD_REQUEST_SEQUENCE = 0

    class OfflineStop(RuntimeError):
        pass

    def stop_before_transport(root):
        authority = json.loads(
            (root / "unit-guard-authority.json").read_text())
        rehearsal = json.loads(
            (root / "unit-guard-rehearsal.json").read_text())
        assert authority["sha256"] == rehearsal["authority_sha256"]
        assert rehearsal["planned_set_equals_authority"] is True
        raise OfflineStop

    monkeypatch.setattr(child, "initialize_ssh_transport", stop_before_transport)
    monkeypatch.setattr(sys, "argv", [
        "halofpx-l13-primary-retry.py",
        "--evidence-dir", str(tmp_path),
        "--l77-primary",
        "--authority-key-file", "/var/tmp/halofpx-l48-control.key",
    ])
    with pytest.raises(OfflineStop):
        child.main()
