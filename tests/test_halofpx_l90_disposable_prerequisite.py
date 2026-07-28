from __future__ import annotations

import importlib.util
from pathlib import Path
import sys

import pytest


SCRIPT = Path(__file__).parents[1] / "scripts" / "halofpx-production-transition.py"
SPEC = importlib.util.spec_from_file_location("halofpx_l90_transition", SCRIPT)
assert SPEC and SPEC.loader
transition = importlib.util.module_from_spec(SPEC)
sys.modules[SPEC.name] = transition
SPEC.loader.exec_module(transition)


def props(unit: str, *, load="not-found", active="inactive", sub="dead",
          pid="0", fragment="", state="", cgroup="") -> str:
    return "\n".join([
        f"Id={unit if load == 'loaded' else ''}",
        f"LoadState={load}", f"ActiveState={active}", f"SubState={sub}",
        f"MainPID={pid}", f"FragmentPath={fragment}",
        f"UnitFileState={state}", f"ControlGroup={cgroup}",
    ]) + "\n"


class Runner:
    def __init__(self, unit_state="absent", *, process_reference=False,
                 path_reference=False, listener=False, path_state="absent",
                 post_stop_failure=False):
        self.unit_state = unit_state
        self.process_reference = process_reference
        self.path_reference = path_reference
        self.listener = listener
        self.path_state = path_state
        self.post_stop_failure = post_stop_failure
        self.stopped = set()
        self.calls = []

    def run(self, host, argv, *, operation="command"):
        argv = list(argv)
        self.calls.append((host, argv, operation))
        if argv[:3] == ["systemctl", "--user", "show"]:
            unit = argv[3]
            if unit in self.stopped and self.post_stop_failure:
                return transition.CommandResult(1, "", "query failed")
            if unit in self.stopped or self.unit_state == "absent":
                return transition.CommandResult(0, props(unit))
            if self.unit_state == "stale":
                return transition.CommandResult(
                    0, props(
                        unit, load="loaded", active="active", sub="exited",
                        fragment=f"/run/user/1000/systemd/transient/{unit}",
                        state="transient"))
            if self.unit_state == "active":
                return transition.CommandResult(
                    0, props(
                        unit, load="loaded", active="active", sub="running",
                        pid="42", fragment=f"/run/user/1000/systemd/transient/{unit}",
                        state="transient", cgroup=f"/user.slice/{unit}"))
            return transition.CommandResult(
                0, props(
                    unit, load="loaded", active="inactive", sub="dead",
                    fragment="/etc/systemd/system/near-name.service",
                    state="enabled"))
        if argv[:3] == ["systemctl", "--user", "stop"]:
            self.stopped.add(argv[3])
            return transition.CommandResult(0, "")
        if argv[:3] == ["ps", "-eo", "pid=,args="]:
            return transition.CommandResult(
                0,
                ("123 /bin/helper halofpx-l50-device-gate.service\n"
                 if self.process_reference else
                 "123 /bin/helper /var/tmp/halofpx-l48-worker\n"
                 if self.path_reference else ""))
        if argv[:3] == ["ss", "-H", "-ltnp"]:
            return transition.CommandResult(
                0, ("LISTEN 0 1 0.0.0.0:50249 0.0.0.0:* pid=123\n"
                    if self.listener else ""))
        if argv[:3] == ["stat", "-c", "%F|%U|%a"]:
            if self.path_state in {"absent", "ambiguous"}:
                return transition.CommandResult(1, "", self.path_state)
            fields = {
                "present": "directory|connorb|700\n",
                "wrong_type": "regular file|connorb|700\n",
                "wrong_owner": "directory|root|700\n",
                "wrong_mode": "directory|connorb|755\n",
                "symlink": "symbolic link|connorb|777\n",
                "mount": "directory|connorb|700\n",
                "referenced": "directory|connorb|700\n",
                "remove_fail": "directory|connorb|700\n",
            }
            return transition.CommandResult(0, fields[self.path_state])
        if argv[:2] == ["test", "!"]:
            return transition.CommandResult(
                0 if self.path_state == "absent" else 1, "")
        if argv[:3] == ["readlink", "-f", "--"]:
            return transition.CommandResult(
                0, ("/different\n" if self.path_state == "symlink"
                    else argv[3] + "\n"))
        if argv[:3] == ["findmnt", "-rn", "-M"]:
            return transition.CommandResult(
                0 if self.path_state == "mount" else 1, "")
        if argv[:3] == ["rm", "-rf", "--"]:
            if self.path_state == "remove_fail":
                return transition.CommandResult(1, "", "refused")
            self.path_state = "absent"
            return transition.CommandResult(0, "")
        if argv[:3] == ["stat", "-c", "%F"]:
            return transition.CommandResult(1, "", "missing")
        raise AssertionError(argv)

    def run_stdin(self, host, argv, stdin, *, operation="command"):
        raise AssertionError("stdin is not used")


def test_stale_active_exited_mainpid_zero_is_unloaded(tmp_path):
    runner = Runner("stale")
    controller = transition.Controller(runner)
    result = controller.reconcile_l90_prerequisite(tmp_path)
    assert result["status"] == "absent"
    assert result["production_mutation_started"] is False
    assert any(record["action"] == "stop_unload" for record in result["records"])
    assert controller.first_mutation is False


def test_genuinely_active_refuses_without_mutation(tmp_path):
    runner = Runner("active")
    controller = transition.Controller(runner)
    with pytest.raises(transition.TransitionError, match="active, owned, or unknown"):
        controller.reconcile_l90_prerequisite(tmp_path)
    assert not any(call[1][:3] == ["systemctl", "--user", "stop"]
                   for call in runner.calls)
    assert controller.first_mutation is False


@pytest.mark.parametrize("kind", ["process", "listener"])
def test_live_reference_or_listener_refuses(kind, tmp_path):
    runner = Runner(
        "stale", process_reference=kind == "process",
        listener=kind == "listener")
    with pytest.raises(transition.TransitionError, match="active, owned, or unknown"):
        transition.Controller(runner).reconcile_l90_prerequisite(tmp_path)
    assert not any(call[1][:3] == ["systemctl", "--user", "stop"]
                   for call in runner.calls)


def test_post_stop_query_failure_refuses(tmp_path):
    runner = Runner("stale", post_stop_failure=True)
    with pytest.raises(transition.TransitionError, match="absence not proven"):
        transition.Controller(runner).reconcile_l90_prerequisite(tmp_path)


def test_unknown_identity_and_near_name_refuse(tmp_path):
    runner = Runner("unknown")
    controller = transition.Controller(runner)
    with pytest.raises(transition.TransitionError, match="unknown disposable identity"):
        controller.reconcile_l90_prerequisite(tmp_path)
    assert all("near-name" not in record for record in (
        unit for units in transition.L90_PREREQUISITE_UNITS.values()
        for unit in units))


def test_absence_is_idempotent(tmp_path):
    for suffix in ("one", "two"):
        root = tmp_path / suffix
        root.mkdir()
        controller = transition.Controller(Runner("absent"))
        result = controller.reconcile_l90_prerequisite(root)
        assert all(record["action"] == "none" for record in result["records"])


def test_ambiguous_stat_failure_refuses(tmp_path):
    runner = Runner("absent", path_state="ambiguous")
    with pytest.raises(transition.TransitionError, match="ambiguous path query"):
        transition.Controller(runner).reconcile_l90_prerequisite(tmp_path)
    assert not any(call[1][:3] == ["rm", "-rf", "--"] for call in runner.calls)


@pytest.mark.parametrize(
    "path_state",
    ["wrong_type", "wrong_owner", "wrong_mode", "symlink", "mount",
     "referenced", "remove_fail"],
)
def test_unsafe_path_identity_or_removal_refuses(path_state, tmp_path):
    runner = Runner(
        "absent", path_state=path_state,
        path_reference=path_state == "referenced")
    with pytest.raises(transition.TransitionError):
        transition.Controller(runner).reconcile_l90_prerequisite(tmp_path)


def test_source_orders_gate_before_shutdown():
    source = SCRIPT.read_text(encoding="utf-8")
    gate = source.index("controller.reconcile_l90_prerequisite(args.evidence_dir)")
    shutdown = source.index("controller.shutdown()", gate)
    assert gate < shutdown
    seam = source[gate:shutdown]
    assert "prepare_l52_evidence_directories" in seam
    assert "prepare_keys" in seam
