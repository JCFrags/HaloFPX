import importlib.util
import hashlib
import json
import sys
import tempfile
import unittest
from pathlib import Path
from unittest import mock


SCRIPT = Path(__file__).parents[1] / "scripts" / "halofpx-production-transition.py"
L22_MANIFEST = Path(__file__).parents[1] / "scripts" / "halofpx-l22-primary-manifest.json"
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
        self.keys = {}
        self.install_mode = "600"
        self.install_owner = transition.CHANNEL_KEY_OWNER
        self.install_type = "regular file"
        self.drop_key_host = None
        self.mismatch_key_host = None
        self.rm_failure_host = None
        self.stat_transport_failure_host = None
        self.key_metadata_stat_count = 0
        self.corrupt_on_metadata_stat = None
        self.existence_probe_failure_host = None
        self.stdin_calls = []
        manifest = json.loads(L22_MANIFEST.read_text(encoding="utf-8"))
        self.remote_hashes = {
            manifest["executables"][name]: manifest["executable_sha256"][name]
            for name in ("worker", "canary", "readiness", "placement")
        }

    def run_stdin(self, host, argv, stdin):
        argv = list(argv)
        self.stdin_calls.append((host, argv))
        if argv[:4] != ["install", "-m", "600", "/dev/stdin"]:
            return transition.CommandResult(99, "", f"unexpected stdin command: {argv}")
        if host != self.drop_key_host:
            content = stdin + (b"x" if host == self.mismatch_key_host else b"")
            self.keys[host] = {
                "path": argv[4],
                "content": content,
                "mode": self.install_mode,
                "owner": self.install_owner,
                "type": self.install_type,
            }
        return transition.CommandResult(0, "")

    def run(self, host, argv):
        argv = list(argv)
        if argv == ["hostname"]:
            return transition.CommandResult(0, self.hostname[host] + "\n")
        if argv[:3] == ["stat", "-c", "%F"]:
            if host == self.existence_probe_failure_host:
                self.existence_probe_failure_host = None
                return transition.CommandResult(255, "", "transport")
            if host == self.stat_transport_failure_host:
                return transition.CommandResult(255, "", "transport")
            key = self.keys.get(host)
            if key and key["path"] == argv[-1]:
                return transition.CommandResult(0, key["type"] + "\n")
            return transition.CommandResult(1, "", "missing")
        if argv[:3] == ["stat", "-c", "%F:%U:%a:%s"]:
            self.key_metadata_stat_count += 1
            if self.key_metadata_stat_count == self.corrupt_on_metadata_stat and host in self.keys:
                self.keys[host]["mode"] = "644"
            key = self.keys.get(host)
            if not key or key["path"] != argv[-1]:
                return transition.CommandResult(1, "", "missing")
            value = f'{key["type"]}:{key["owner"]}:{key["mode"]}:{len(key["content"])}\n'
            return transition.CommandResult(0, value)
        if argv[:2] == ["sha256sum", "--"]:
            if argv[-1] in self.remote_hashes:
                digest = self.remote_hashes[argv[-1]]
                return transition.CommandResult(0, f"{digest}  {argv[-1]}\n")
            key = self.keys.get(host)
            if not key or key["path"] != argv[-1]:
                return transition.CommandResult(1, "", "missing")
            digest = hashlib.sha256(key["content"]).hexdigest()
            return transition.CommandResult(0, f"{digest}  {argv[-1]}\n")
        if argv[:3] == ["rm", "-f", "--"]:
            if host == self.rm_failure_host:
                return transition.CommandResult(1, "", "remove failed")
            self.keys.pop(host, None)
            return transition.CommandResult(0, "")
        if argv[:3] == ["rm", "-rf", "--"]:
            return transition.CommandResult(0, "")
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
        controller.prepare_keys()
        controller.shutdown()
        controller.recover()
        self.assertEqual(fake.mutations, [
            ("nimo-1", "stop", transition.COORDINATOR_UNIT),
            ("nimo-2", "stop", transition.WORKER_UNIT),
            ("nimo-2", "start", transition.WORKER_UNIT),
            ("nimo-1", "start", transition.COORDINATOR_UNIT),
        ])

    def test_key_provision_is_identical_exact_and_consumable(self):
        fake = FakeRunner()
        controller = self.controller(fake)
        receipt = controller.prepare_keys()
        self.assertEqual(receipt["bytes"], 130)
        self.assertEqual(receipt["hosts"]["nimo-1"]["mode"], "600")
        self.assertEqual(
            receipt["hosts"]["nimo-1"]["sha256"],
            receipt["hosts"]["nimo-2"]["sha256"],
        )
        self.assertEqual(controller.validate_keys(), receipt["hosts"])
        self.assertEqual(fake.keys["nimo-1"]["content"], fake.keys["nimo-2"]["content"])

    def test_bad_key_mode_refuses_before_first_mutation(self):
        fake = FakeRunner()
        fake.install_mode = "644"
        controller = self.controller(fake)
        controller.preflight()
        with self.assertRaises(transition.TransitionError):
            controller.prepare_keys()
        self.assertFalse(controller.first_mutation)
        self.assertEqual(fake.mutations, [])


    def test_key_missing_mismatch_and_type_refuse(self):
        for attribute, value in (
            ("drop_key_host", "nimo-2"),
            ("mismatch_key_host", "nimo-2"),
            ("install_type", "symbolic link"),
        ):
            with self.subTest(attribute=attribute):
                fake = FakeRunner()
                setattr(fake, attribute, value)
                controller = self.controller(fake)
                controller.preflight()
                with self.assertRaises(transition.TransitionError):
                    controller.prepare_keys()
                self.assertFalse(controller.first_mutation)
                self.assertEqual(fake.mutations, [])
                self.assertEqual(fake.keys, {})

    def test_key_freshness_transport_failure_installs_nothing(self):
        fake = FakeRunner()
        fake.existence_probe_failure_host = "nimo-1"
        controller = self.controller(fake)
        controller.preflight()
        with self.assertRaises(transition.TransitionError):
            controller.prepare_keys()
        self.assertEqual(fake.stdin_calls, [])
        self.assertEqual(fake.keys, {})
        self.assertFalse(controller.first_mutation)
        self.assertEqual(fake.mutations, [])

    def test_key_secret_is_not_exposed_by_receipt_or_error(self):
        fake = FakeRunner()
        controller = self.controller(fake)
        receipt = controller.prepare_keys()
        secret = fake.keys["nimo-1"]["content"]
        self.assertNotIn(secret.decode("ascii"), json.dumps(receipt))
        fake.keys["nimo-2"]["mode"] = "644"
        with self.assertRaises(transition.TransitionError) as raised:
            controller.validate_keys()
        self.assertNotIn(secret.decode("ascii"), str(raised.exception))

    def test_changed_key_refuses_at_last_pre_mutation_check(self):
        fake = FakeRunner()
        controller = self.controller(fake)
        controller.preflight()
        controller.prepare_keys()
        fake.keys["nimo-2"]["content"] += b"x"
        with self.assertRaises(transition.TransitionError):
            controller.shutdown()
        self.assertFalse(controller.first_mutation)
        self.assertEqual(fake.mutations, [])

    def test_key_cleanup_refuses_failed_remove(self):
        fake = FakeRunner()
        controller = self.controller(fake)
        controller.prepare_keys()
        fake.rm_failure_host = "nimo-1"
        with self.assertRaises(transition.TransitionError):
            controller.cleanup_keys()
        self.assertIn("nimo-1", fake.keys)

    def test_key_cleanup_refuses_failed_absence_transport(self):
        fake = FakeRunner()
        controller = self.controller(fake)
        controller.prepare_keys()
        fake.stat_transport_failure_host = "nimo-2"
        with self.assertRaises(transition.TransitionError):
            controller.cleanup_keys()

    def test_key_receipt_failure_cleans_before_mutation(self):
        fake = FakeRunner()
        with tempfile.TemporaryDirectory() as directory:
            evidence = Path(directory).resolve()
            (evidence / "key-preparation.json").write_text("sentinel", encoding="utf-8")
            result = transition.main([
                "--evidence-dir", str(evidence),
                "maintenance", "--", sys.executable, "-c", "raise SystemExit(0)",
            ], runner=fake)
        self.assertEqual(result, 1)
        self.assertEqual(fake.keys, {})
        self.assertEqual(fake.mutations, [])

    def test_last_key_revalidation_failure_cleans_before_mutation(self):
        fake = FakeRunner()
        fake.corrupt_on_metadata_stat = 3
        with tempfile.TemporaryDirectory() as directory:
            result = transition.main([
                "--evidence-dir", str(Path(directory).resolve()),
                "maintenance", "--", sys.executable, "-c", "raise SystemExit(0)",
            ], runner=fake)
        self.assertEqual(result, 1)
        self.assertEqual(fake.keys, {})
        self.assertEqual(fake.mutations, [])

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
        controller.prepare_keys()
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
        controller.prepare_keys()
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

    def test_recovery_stops_and_verifies_orphaned_l16_unit_first(self):
        fake = FakeRunner()
        fake.disposable_active.add(transition.DISPOSABLE_WORKER_UNITS[1])
        fake.disposable_port_open = True
        controller = self.controller(fake)
        controller.preflight()
        controller.prepare_keys()
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
        controller.prepare_keys()
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
        controller.prepare_keys()
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


class ManifestBindingTests(unittest.TestCase):
    def setUp(self):
        self.manifest = json.loads(L22_MANIFEST.read_text(encoding="utf-8"))
        self.temp = tempfile.TemporaryDirectory()
        self.evidence = Path(self.temp.name).resolve()
        self.expected = [
            self.manifest["executables"]["interpreter"],
            self.manifest["executables"]["child"],
            "--evidence-dir",
            str(self.evidence / "child"),
        ]

    def tearDown(self):
        self.temp.cleanup()

    def test_exact_command_is_bound_for_popen(self):
        self.assertEqual(
            transition.bind_maintenance_command(
                self.manifest, self.evidence, ["--", *self.expected],
            ),
            self.expected,
        )

    def test_wrong_extra_missing_and_reordered_argv_refuse(self):
        variants = [
            ["C:\\Windows\\System32\\cmd.exe", *self.expected[1:]],
            [*self.expected, "--extra"],
            self.expected[:-1],
            [self.expected[0], self.expected[1], self.expected[3], self.expected[2]],
        ]
        for argv in variants:
            with self.subTest(argv=argv), self.assertRaises(transition.TransitionError):
                transition.bind_maintenance_command(self.manifest, self.evidence, argv)

    def test_wrong_evidence_root_refuses(self):
        argv = [*self.expected[:3], str(self.evidence.parent / "outside")]
        with self.assertRaises(transition.TransitionError):
            transition.bind_maintenance_command(self.manifest, self.evidence, argv)

    def test_separator_normalization_is_the_only_path_normalization(self):
        argv = [
            self.expected[0].replace("\\", "/"),
            self.expected[1].replace("\\", "/"),
            "--evidence-dir",
            self.expected[3].replace("\\", "/"),
        ]
        self.assertEqual(
            transition.bind_maintenance_command(self.manifest, self.evidence, argv),
            self.expected,
        )

    def test_wrong_hash_refuses_manifest_before_mutation(self):
        altered = json.loads(json.dumps(self.manifest))
        altered["executable_sha256"]["child"] = "0" * 64
        path = self.evidence / "manifest.json"
        path.write_text(json.dumps(altered), encoding="utf-8")
        fake = FakeRunner()
        with self.assertRaises(transition.TransitionError):
            transition.validate_milestone_manifest(path, fake)
        self.assertEqual(fake.mutations, [])

    def test_bound_command_is_the_exact_popen_argv(self):
        fake = FakeRunner()
        child = mock.Mock()
        child.wait.return_value = 0
        with mock.patch.object(transition.subprocess, "Popen", return_value=child) as popen:
            result = transition.main([
                "--evidence-dir", str(self.evidence),
                "--milestone-manifest", str(L22_MANIFEST),
                "--timeout-seconds", "0.1",
                "maintenance", "--", *self.expected,
            ], runner=fake)
        self.assertEqual(result, 0)
        self.assertEqual(popen.call_args.args[0], self.expected)
        self.assertEqual(fake.mutations, [
            ("nimo-1", "stop", transition.COORDINATOR_UNIT),
            ("nimo-2", "stop", transition.WORKER_UNIT),
            ("nimo-2", "start", transition.WORKER_UNIT),
            ("nimo-1", "start", transition.COORDINATOR_UNIT),
        ])


if __name__ == "__main__":
    unittest.main()
