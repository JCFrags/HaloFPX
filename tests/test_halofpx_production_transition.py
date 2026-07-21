import importlib.util
import json
import sys
import tempfile
import unittest
from pathlib import Path


SCRIPT = Path(__file__).parents[1] / "scripts" / "halofpx-production-transition.py"
SPEC = importlib.util.spec_from_file_location("halofpx_transition", SCRIPT)
assert SPEC and SPEC.loader
transition = importlib.util.module_from_spec(SPEC)
sys.modules[SPEC.name] = transition
SPEC.loader.exec_module(transition)


class FakeRunner:
    def __init__(self):
        self.hostname = {"nimo-1": "nimo-1", "nimo-2": "nimo-2"}
        self.active = {"nimo-1": True, "nimo-2": True}
        self.port_open = {"nimo-1": True, "nimo-2": True}
        self.pid = {"nimo-1": 101, "nimo-2": 202}
        self.listener_pid = {"nimo-1": 101, "nimo-2": 202}
        self.units = {
            "nimo-1": transition.COORDINATOR_UNIT,
            "nimo-2": transition.WORKER_UNIT,
        }
        self.execs = {
            "nimo-1": transition.COORDINATOR_EXEC,
            "nimo-2": transition.WORKER_EXEC,
        }
        self.commands = {
            "nimo-1": " ".join(transition.COORDINATOR.expected_process_tokens),
            "nimo-2": " ".join(transition.WORKER.expected_process_tokens),
        }
        self.partial = set()
        self.keep_coordinator_listener = False
        self.ignore_coordinator_stop = False
        self.deactivating_coordinator = False
        self.state_override = {}
        self.mutations = []
        self.disposable_active = set()
        self.disposable_port_open = False
        self.disposable_mutations = []
        self.ignore_disposable_stop = False
        self.canary_process_active = False

    def run(self, host, argv):
        argv = list(argv)
        if argv == ["hostname"]:
            return transition.CommandResult(0, self.hostname[host] + "\n")
        if argv[:2] == ["systemctl", "show"]:
            if host in self.partial:
                return transition.CommandResult(0, "Id=broken\n")
            active = self.active[host]
            state = self.state_override.get(host)
            active_state = state[0] if state else ("active" if active else "inactive")
            sub_state = state[1] if state else ("running" if active else "dead")
            main_pid = state[2] if state else (self.pid[host] if active else 0)
            unit = self.units[host]
            text = "\n".join((
                f"Id={unit}",
                "LoadState=loaded",
                f"ActiveState={active_state}",
                f"SubState={sub_state}",
                f"MainPID={main_pid}",
                f"ExecStart={{ path={self.execs[host]} ; argv[]={self.execs[host]} ; }}",
                f"FragmentPath=/etc/systemd/system/{unit}",
                "NRestarts=0",
                "ExecMainStartTimestamp=test",
            ))
            return transition.CommandResult(0, text + "\n")
        if argv[:3] == ["systemctl", "--user", "show"]:
            unit = argv[3]
            active = unit in self.disposable_active
            text = "\n".join((
                "LoadState=loaded" if active else "LoadState=not-found",
                "ActiveState=active" if active else "ActiveState=inactive",
                "SubState=running" if active else "SubState=dead",
                "MainPID=303" if active else "MainPID=0",
            ))
            return transition.CommandResult(0, text + "\n")
        if argv[:2] == ["ps", "-p"]:
            return transition.CommandResult(0, self.commands[host] + "\n")
        if argv == ["ps", "-eo", "pid=,args="]:
            if host == transition.DISPOSABLE_CANARY_HOST and self.canary_process_active:
                return transition.CommandResult(
                    0, f"404 {transition.DISPOSABLE_CANARY_BIN} --hfx-mode capture\n"
                )
            return transition.CommandResult(0, "")
        if argv[:3] == ["ss", "-H", "-ltnp"]:
            lines = []
            if self.port_open[host]:
                port = 8081 if host == "nimo-1" else 50052
                process = "llama-server" if host == "nimo-1" else "ggml-rpc-server"
                lines.append(f'LISTEN 0 1 0.0.0.0:{port} 0.0.0.0:* users:(("{process}",pid={self.listener_pid[host]},fd=5))')
            if host == transition.DISPOSABLE_HOST and self.disposable_port_open:
                lines.append(f'LISTEN 0 1 10.44.0.1:{transition.DISPOSABLE_PORT} 0.0.0.0:* users:(("rpc-server",pid=303,fd=7))')
            return transition.CommandResult(0, "\n".join(lines) + ("\n" if lines else ""))
        if argv and argv[0] == "curl":
            return transition.CommandResult(0, "200")
        if argv[:3] == ["sudo", "-n", "systemctl"]:
            verb, unit = argv[3], argv[4]
            self.mutations.append((host, verb, unit))
            if verb == "stop":
                if host == "nimo-1" and self.deactivating_coordinator:
                    self.active[host] = False
                    self.port_open[host] = False
                    self.state_override[host] = ("deactivating", "stop-sigterm", self.pid[host])
                elif not (host == "nimo-1" and self.ignore_coordinator_stop):
                    self.active[host] = False
                if not (host == "nimo-1" and (self.keep_coordinator_listener or self.ignore_coordinator_stop)):
                    self.port_open[host] = False
            else:
                self.state_override.pop(host, None)
                self.active[host] = True
                self.port_open[host] = True
                self.pid[host] += 1000
                self.listener_pid[host] = self.pid[host]
            return transition.CommandResult(0, "")
        if argv[:3] == ["systemctl", "--user", "stop"]:
            unit = argv[3]
            self.disposable_mutations.append((host, "stop", unit))
            if not self.ignore_disposable_stop:
                self.disposable_active.discard(unit)
                if unit in transition.DISPOSABLE_CANARY_UNITS:
                    self.canary_process_active = False
            if not self.disposable_active:
                self.disposable_port_open = False
            return transition.CommandResult(0, "")
        return transition.CommandResult(99, "", f"unexpected command: {argv}")


class ControllerTests(unittest.TestCase):
    def controller(self, fake):
        return transition.Controller(fake, wait_seconds=0, timeout_seconds=0.1)

    def test_preflight_and_ordered_shutdown_recovery(self):
        fake = FakeRunner()
        controller = self.controller(fake)
        controller.preflight()
        controller.shutdown()
        controller.recover()
        self.assertEqual(fake.mutations, [
            ("nimo-1", "stop", transition.COORDINATOR_UNIT),
            ("nimo-2", "stop", transition.WORKER_UNIT),
            ("nimo-2", "start", transition.WORKER_UNIT),
            ("nimo-1", "start", transition.COORDINATOR_UNIT),
        ])

    def test_swapped_hostname_refuses_without_mutation(self):
        fake = FakeRunner()
        fake.hostname["nimo-1"] = "nimo-2"
        with self.assertRaises(transition.TransitionError):
            self.controller(fake).preflight()
        self.assertEqual(fake.mutations, [])

    def test_swapped_unit_refuses_without_mutation(self):
        fake = FakeRunner()
        fake.units["nimo-1"] = transition.WORKER_UNIT
        with self.assertRaises(transition.TransitionError):
            self.controller(fake).preflight()
        self.assertEqual(fake.mutations, [])

    def test_unexpected_listener_pid_refuses_without_mutation(self):
        fake = FakeRunner()
        fake.listener_pid["nimo-1"] = 999
        with self.assertRaises(transition.TransitionError):
            self.controller(fake).preflight()
        self.assertEqual(fake.mutations, [])

    def test_unexpected_process_command_refuses_without_mutation(self):
        fake = FakeRunner()
        fake.commands["nimo-1"] = "/bin/false --port 8081"
        with self.assertRaises(transition.TransitionError):
            self.controller(fake).preflight()
        self.assertEqual(fake.mutations, [])

    def test_unexpected_argument_drift_refuses_without_mutation(self):
        fake = FakeRunner()
        fake.commands["nimo-1"] = fake.commands["nimo-1"].replace(
            "--tensor-split 1,1", "--tensor-split 2,0"
        )
        with self.assertRaises(transition.TransitionError):
            self.controller(fake).preflight()
        self.assertEqual(fake.mutations, [])

    def test_execstart_suffix_drift_refuses_without_mutation(self):
        fake = FakeRunner()
        fake.execs["nimo-1"] = "/unexpected" + transition.COORDINATOR_EXEC
        with self.assertRaises(transition.TransitionError):
            self.controller(fake).preflight()
        self.assertEqual(fake.mutations, [])

    def test_partial_preflight_refuses_without_mutation(self):
        fake = FakeRunner()
        fake.partial.add("nimo-2")
        with self.assertRaises(transition.TransitionError):
            self.controller(fake).preflight()
        self.assertEqual(fake.mutations, [])

    def test_coordinator_listener_blocks_worker_stop(self):
        fake = FakeRunner()
        fake.keep_coordinator_listener = True
        controller = self.controller(fake)
        controller.preflight()
        with self.assertRaises(transition.TransitionError):
            controller.shutdown()
        self.assertEqual(fake.mutations, [
            ("nimo-1", "stop", transition.COORDINATOR_UNIT),
        ])

    def test_coordinator_still_active_blocks_worker_stop(self):
        fake = FakeRunner()
        fake.ignore_coordinator_stop = True
        controller = self.controller(fake)
        controller.preflight()
        with self.assertRaises(transition.TransitionError):
            controller.shutdown()
        self.assertEqual(fake.mutations, [
            ("nimo-1", "stop", transition.COORDINATOR_UNIT),
        ])

    def test_shutdown_without_complete_preflight_refuses(self):
        fake = FakeRunner()
        with self.assertRaises(transition.TransitionError):
            self.controller(fake).shutdown()
        self.assertEqual(fake.mutations, [])

    def test_abnormal_maintenance_exit_runs_ordered_recovery(self):
        fake = FakeRunner()
        with tempfile.TemporaryDirectory() as directory:
            result = transition.main([
                "--evidence-dir", str(Path(directory).resolve()),
                "--timeout-seconds", "0.1",
                "maintenance", "--", sys.executable, "-c", "raise SystemExit(7)",
            ], runner=fake)
        self.assertEqual(result, 1)
        self.assertEqual(fake.mutations, [
            ("nimo-1", "stop", transition.COORDINATOR_UNIT),
            ("nimo-2", "stop", transition.WORKER_UNIT),
            ("nimo-2", "start", transition.WORKER_UNIT),
            ("nimo-1", "start", transition.COORDINATOR_UNIT),
        ])

    def test_recovery_stops_and_verifies_orphaned_l15_unit_first(self):
        fake = FakeRunner()
        fake.disposable_active.add(transition.DISPOSABLE_WORKER_UNITS[1])
        fake.disposable_port_open = True
        controller = self.controller(fake)
        controller.preflight()
        controller.shutdown()
        controller.recover()
        self.assertEqual(
            fake.disposable_mutations,
            [
                (host, "stop", unit)
                for host, units in (
                    ("nimo-1", transition.DISPOSABLE_WORKER_UNITS),
                    ("nimo-2", transition.DISPOSABLE_CANARY_UNITS),
                )
                for unit in units
            ],
        )
        self.assertEqual(fake.mutations[-2:], [
            ("nimo-2", "start", transition.WORKER_UNIT),
            ("nimo-1", "start", transition.COORDINATOR_UNIT),
        ])

    def test_unstoppable_orphan_blocks_all_production_starts(self):
        fake = FakeRunner()
        fake.disposable_active.add(transition.DISPOSABLE_WORKER_UNITS[0])
        fake.disposable_port_open = True
        fake.ignore_disposable_stop = True
        controller = self.controller(fake)
        controller.preflight()
        controller.shutdown()
        with self.assertRaises(transition.TransitionError):
            controller.recover()
        self.assertFalse(any(verb == "start" for _, verb, _ in fake.mutations))

    def test_unstoppable_remote_canary_blocks_all_production_starts(self):
        fake = FakeRunner()
        fake.disposable_active.add(transition.DISPOSABLE_CANARY_UNITS[0])
        fake.canary_process_active = True
        fake.ignore_disposable_stop = True
        controller = self.controller(fake)
        controller.preflight()
        controller.shutdown()
        with self.assertRaises(transition.TransitionError):
            controller.recover()
        self.assertFalse(any(verb == "start" for _, verb, _ in fake.mutations))

    def test_deactivating_coordinator_runs_rollback_without_worker_stop(self):
        fake = FakeRunner()
        fake.deactivating_coordinator = True
        with tempfile.TemporaryDirectory() as directory:
            result = transition.main([
                "--evidence-dir", str(Path(directory).resolve()),
                "--timeout-seconds", "0.1",
                "maintenance", "--", sys.executable, "-c", "raise SystemExit(0)",
            ], runner=fake)
        self.assertEqual(result, 1)
        self.assertEqual(fake.mutations, [
            ("nimo-1", "stop", transition.COORDINATOR_UNIT),
            ("nimo-2", "start", transition.WORKER_UNIT),
            ("nimo-1", "start", transition.COORDINATOR_UNIT),
        ])

    def test_tampered_recovery_snapshot_refuses_before_mutation(self):
        fake = FakeRunner()
        controller = self.controller(fake)
        snapshot = transition._snapshot_dict(controller.preflight())
        snapshot["roles"]["coordinator"]["host"] = "nimo-2"
        with tempfile.TemporaryDirectory() as directory:
            evidence = Path(directory).resolve()
            (evidence / "production-preflight.json").write_text(
                json.dumps(snapshot), encoding="utf-8"
            )
            result = transition.main([
                "--evidence-dir", str(evidence), "recover",
            ], runner=fake)
        self.assertEqual(result, 1)
        self.assertEqual(fake.mutations, [])

    def test_preserved_snapshot_is_not_overwritten(self):
        fake = FakeRunner()
        with tempfile.TemporaryDirectory() as directory:
            evidence = Path(directory).resolve()
            snapshot = evidence / "production-preflight.json"
            snapshot.write_text("sentinel", encoding="utf-8")
            result = transition.main([
                "--evidence-dir", str(evidence), "preflight",
            ], runner=fake)
            self.assertEqual(snapshot.read_text(encoding="utf-8"), "sentinel")
        self.assertEqual(result, 1)
        self.assertEqual(fake.mutations, [])


if __name__ == "__main__":
    unittest.main()
