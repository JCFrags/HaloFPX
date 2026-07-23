import importlib.util
import hashlib
import json
import os
import subprocess
import sys
import tempfile
import time
import unittest
from pathlib import Path
from unittest import mock


SCRIPT = Path(__file__).parents[1] / "scripts" / "halofpx-production-transition.py"
L22_MANIFEST = Path(__file__).parents[1] / "scripts" / "halofpx-l24-primary-manifest.json"
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

    def run_stdin(self, host, argv, stdin, *, operation="command"):
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

    def run(self, host, argv, *, operation="command"):
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


class NeverReturningProcess:
    next_pid = 9000

    def __init__(self, *, stdout=b"", stderr=b"", ignore_term=False):
        self.pid = NeverReturningProcess.next_pid
        NeverReturningProcess.next_pid += 1
        self.returncode = None
        self.stdout_prefix = stdout
        self.stderr_prefix = stderr
        self.ignore_term = ignore_term
        self.terminated = False
        self.wait_calls = 0

    def communicate(self, input=None, timeout=None):
        if timeout is not None:
            raise transition.subprocess.TimeoutExpired(
                ["ssh"], timeout, output=self.stdout_prefix, stderr=self.stderr_prefix)
        self.returncode = -9 if self.ignore_term else -15
        return b"", b""

    def poll(self):
        return self.returncode

    def terminate(self):
        self.terminated = True
        if not self.ignore_term:
            self.returncode = -15

    def wait(self, timeout=None):
        self.wait_calls += 1
        if self.returncode is not None:
            return self.returncode
        if self.ignore_term and self.wait_calls == 1:
            raise transition.subprocess.TimeoutExpired(["ssh"], timeout)
        self.returncode = -9
        return self.returncode


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


class BoundedSshRunnerTests(unittest.TestCase):
    def make_runner(self, root, process):
        deadlines = dict(transition.SSH_OPERATION_DEADLINES)
        deadlines["command"] = 0.01
        deadlines["recovery-probe"] = 0.01
        return transition.SshRunner(
            Path(root), deadlines=deadlines,
            popen_factory=lambda *args, **kwargs: process,
        )

    def test_never_returning_ssh_is_typed_reaped_and_evidenced(self):
        with tempfile.TemporaryDirectory() as tmp:
            process = NeverReturningProcess()
            runner = self.make_runner(tmp, process)
            with self.assertRaises(transition.SshTimeoutError) as caught:
                runner.run("nimo-1", ["hostname"], operation="command")
            self.assertEqual(caught.exception.operation, "command")
            self.assertIsNotNone(process.poll())
            record = json.loads((Path(tmp) / "ssh-operations.jsonl").read_text())
            self.assertTrue(record["timed_out"])
            self.assertTrue(record["term_sent"])
            self.assertFalse(record["kill_sent"])
            self.assertEqual(record["host"], "nimo-1")
            self.assertEqual(record["argv"], ["hostname"])

    def test_output_then_hang_retains_bounded_output(self):
        with tempfile.TemporaryDirectory() as tmp:
            process = NeverReturningProcess(stdout=b"ready\n", stderr=b"partial\n")
            runner = self.make_runner(tmp, process)
            with self.assertRaises(transition.SshTimeoutError):
                runner.run("nimo-2", ["test", "-f", "/x"], operation="command")
            record = json.loads((Path(tmp) / "ssh-operations.jsonl").read_text())
            self.assertIn("ready", record["stdout"])
            self.assertIn("partial", record["stderr"])
            self.assertLessEqual(len(record["stdout"]), transition.SSH_EVIDENCE_LIMIT)

    def test_term_ignoring_process_escalates_and_is_reaped(self):
        with tempfile.TemporaryDirectory() as tmp:
            process = NeverReturningProcess(ignore_term=True)
            runner = self.make_runner(tmp, process)
            with self.assertRaises(transition.SshTimeoutError):
                runner.run("nimo-1", ["hostname"], operation="command")
            self.assertIsNotNone(process.poll())
            record = json.loads((Path(tmp) / "ssh-operations.jsonl").read_text())
            self.assertTrue(record["kill_sent"])

    @unittest.skipUnless(os.name == "nt", "Windows job-object qualification")
    def test_real_timeout_job_leaves_no_descendant(self):
        with tempfile.TemporaryDirectory() as tmp:
            child_pid_path = Path(tmp) / "child.pid"
            child_code = "import time; time.sleep(30)"
            parent_code = (
                "import pathlib,subprocess,sys,time;"
                f"p=subprocess.Popen([sys.executable,'-c',{child_code!r}]);"
                f"pathlib.Path({str(child_pid_path)!r}).write_text(str(p.pid));"
                "time.sleep(30)"
            )

            def factory(_command, **kwargs):
                return subprocess.Popen([sys.executable, "-c", parent_code], **kwargs)

            deadlines = dict(transition.SSH_OPERATION_DEADLINES)
            deadlines["command"] = 1.0
            runner = transition.SshRunner(
                Path(tmp), deadlines=deadlines, popen_factory=factory)
            with self.assertRaises(transition.SshTimeoutError):
                runner.run("nimo-1", ["hostname"], operation="command")
            child_pid = int(child_pid_path.read_text())
            time.sleep(0.1)
            query = subprocess.run(
                ["tasklist", "/FI", f"PID eq {child_pid}", "/FO", "CSV", "/NH"],
                text=True, capture_output=True, check=False, timeout=5)
            self.assertNotIn(f'"{child_pid}"', query.stdout)

    def test_unknown_operation_class_is_refused_before_spawn(self):
        with tempfile.TemporaryDirectory() as tmp:
            process = NeverReturningProcess()
            runner = self.make_runner(tmp, process)
            with self.assertRaises(ValueError):
                runner.run("nimo-1", ["hostname"], operation="not-authorized")
            self.assertIsNone(process.poll())

    def test_process_group_setup_failure_reaps_and_is_evidenced(self):
        with tempfile.TemporaryDirectory() as tmp:
            process = NeverReturningProcess()
            runner = self.make_runner(tmp, process)
            with mock.patch.object(
                runner, "_create_windows_job", side_effect=OSError("job refused")
            ):
                with self.assertRaises(transition.SshSetupError):
                    runner.run("nimo-1", ["hostname"], operation="command")
            self.assertIsNotNone(process.poll())
            record = json.loads((Path(tmp) / "ssh-operations.jsonl").read_text())
            self.assertEqual(record["failure_class"], "process-group-setup")
            self.assertTrue(record["term_sent"])

    @unittest.skipUnless(os.name == "nt", "Windows taskkill-timeout qualification")
    def test_setup_failure_taskkill_timeout_still_reaps_and_records_fatal_gap(self):
        with tempfile.TemporaryDirectory() as tmp:
            process = NeverReturningProcess()
            runner = self.make_runner(tmp, process)
            with mock.patch.object(
                runner, "_create_windows_job", side_effect=OSError("job refused")
            ), mock.patch.object(
                transition.subprocess, "run",
                side_effect=transition.subprocess.TimeoutExpired(["taskkill"], 5),
            ):
                with self.assertRaises(transition.SshSetupError):
                    runner.run("nimo-1", ["hostname"], operation="command")
            self.assertIsNotNone(process.poll())
            record = json.loads((Path(tmp) / "ssh-operations.jsonl").read_text())
            self.assertEqual(record["failure_class"], "process-group-setup")
            self.assertIn("taskkill_error=TimeoutExpired", record["stderr"])
            self.assertIn("descendant_cleanup_unproven", record["stderr"])

    @unittest.skipUnless(os.name == "nt", "Windows setup-failure tree qualification")
    def test_real_setup_failure_tree_kill_leaves_no_descendant(self):
        with tempfile.TemporaryDirectory() as tmp:
            child_pid_path = Path(tmp) / "setup-child.pid"
            child_code = "import time; time.sleep(30)"
            parent_code = (
                "import pathlib,subprocess,sys,time;"
                f"p=subprocess.Popen([sys.executable,'-c',{child_code!r}]);"
                f"pathlib.Path({str(child_pid_path)!r}).write_text(str(p.pid));"
                "time.sleep(30)"
            )

            def factory(_command, **kwargs):
                process = subprocess.Popen([sys.executable, "-c", parent_code], **kwargs)
                deadline = time.monotonic() + 2
                while not child_pid_path.exists() and time.monotonic() < deadline:
                    time.sleep(0.01)
                return process

            runner = transition.SshRunner(Path(tmp), popen_factory=factory)
            with mock.patch.object(
                runner, "_create_windows_job", side_effect=OSError("forced setup failure")
            ):
                with self.assertRaises(transition.SshSetupError):
                    runner.run("nimo-1", ["hostname"], operation="command")
            child_pid = int(child_pid_path.read_text())
            query = subprocess.run(
                ["tasklist", "/FI", f"PID eq {child_pid}", "/FO", "CSV", "/NH"],
                text=True, capture_output=True, check=False, timeout=5)
            self.assertNotIn(f'"{child_pid}"', query.stdout)
            record = json.loads((Path(tmp) / "ssh-operations.jsonl").read_text())
            self.assertTrue(record["kill_sent"])

    def test_transport_failure_classes_remain_distinguishable(self):
        cases = {
            "Host key verification failed.": "host-key",
            "ssh: connect to host x port 22: Connection timed out": "connect",
            "Permission denied (publickey).": "authentication",
            "remote command failed": "command",
        }
        for stderr, expected in cases.items():
            with self.subTest(stderr=stderr):
                self.assertEqual(transition.SshRunner._classify_failure(stderr), expected)

    def test_recovery_probe_timeout_is_distinguishable_and_recoverable(self):
        class TimeoutOnceRunner(FakeRunner):
            def __init__(self):
                super().__init__()
                self.timed_out = False

            def run(self, host, argv, *, operation="command"):
                if operation == "recovery-probe" and not self.timed_out:
                    self.timed_out = True
                    raise transition.SshTimeoutError(host, operation, 0.01)
                return super().run(host, argv, operation=operation)

        fake = TimeoutOnceRunner()
        controller = transition.Controller(fake, wait_seconds=0, timeout_seconds=1)
        controller.snapshot = controller.preflight()
        controller.first_mutation = True
        with self.assertRaises(transition.TransitionError) as caught:
            controller.recover()
        self.assertIn("typed SSH timeout operation=recovery-probe", str(caught.exception))
        final = controller.recover()
        self.assertEqual(final["worker"].main_pid, fake.pid["nimo-2"])
        self.assertTrue(controller.recovery_complete)
        self.assertEqual(fake.mutations, [
            ("nimo-2", "start", transition.WORKER_UNIT),
            ("nimo-1", "start", transition.COORDINATOR_UNIT),
        ])


if __name__ == "__main__":
    unittest.main()
