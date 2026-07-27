from __future__ import annotations

import importlib.util
import json
import os
from pathlib import Path
import subprocess
import sys
import tempfile
import unittest
from unittest import mock


SCRIPT = Path(__file__).parents[1] / "scripts" / "halofpx-production-transition.py"
SPEC = importlib.util.spec_from_file_location("halofpx_l59_transition", SCRIPT)
assert SPEC and SPEC.loader
transition = importlib.util.module_from_spec(SPEC)
sys.modules[SPEC.name] = transition
SPEC.loader.exec_module(transition)


class BoundedEvidenceTransferTests(unittest.TestCase):
    def test_receive_file_is_bounded_and_never_logs_payload(self):
        payload = b"authenticated-private-prefix\n"
        with tempfile.TemporaryDirectory() as temporary:
            root = Path(temporary).resolve()

            def factory(_command, **kwargs):
                return subprocess.Popen(
                    [sys.executable, "-c",
                     "import os; os.write(1, b'authenticated-private-prefix\\n')"],
                    **kwargs)

            runner = transition.SshRunner(root, popen_factory=factory)
            target = root / "received"
            result = runner.receive_file(
                "nimo-1", "/absolute/source", target,
                expected_size=len(payload), operation="evidence")
            self.assertEqual(result.returncode, 0)
            self.assertEqual(target.read_bytes(), payload)
            record = json.loads(runner.evidence_path.read_text().splitlines()[-1])
            self.assertEqual(record["stdout"], "")
            self.assertEqual(record["stdout_bytes"], len(payload))
            self.assertNotIn(payload.decode().strip(), runner.evidence_path.read_text())

    @unittest.skipIf(os.name == "nt", "POSIX process-group assertion")
    def test_receive_file_timeout_reaps_process_group_and_retains_record(self):
        with tempfile.TemporaryDirectory() as temporary:
            root = Path(temporary).resolve()
            deadlines = dict(transition.SSH_OPERATION_DEADLINES)
            deadlines["evidence"] = 0.05

            def factory(_command, **kwargs):
                return subprocess.Popen(
                    [sys.executable, "-c",
                     "import os,time; os.write(1,b'x'); time.sleep(30)"],
                    **kwargs)

            runner = transition.SshRunner(
                root, deadlines=deadlines, popen_factory=factory)
            with self.assertRaises(transition.SshTimeoutError):
                runner.receive_file(
                    "nimo-1", "/absolute/source", root / "partial",
                    expected_size=1, operation="evidence")
            record = json.loads(runner.evidence_path.read_text().splitlines()[-1])
            self.assertTrue(record["timed_out"])
            self.assertTrue(record["term_sent"])
            self.assertEqual(record["stdout_bytes"], 1)

    def test_receive_file_refuses_unbounded_or_relative_authority_before_spawn(self):
        with tempfile.TemporaryDirectory() as temporary:
            root = Path(temporary).resolve()
            runner = transition.SshRunner(
                root, popen_factory=lambda *_args, **_kwargs: self.fail("spawned"))
            with self.assertRaises(ValueError):
                runner.receive_file(
                    "nimo-1", "relative", root / "a",
                    expected_size=1, operation="evidence")
            with self.assertRaises(ValueError):
                runner.receive_file(
                    "nimo-1", "/absolute", root / "b",
                    expected_size=65537, operation="evidence")

    def test_receive_file_retains_process_setup_failure(self):
        with tempfile.TemporaryDirectory() as temporary:
            root = Path(temporary).resolve()

            def fail_setup(*_args, **_kwargs):
                raise OSError("injected setup")

            runner = transition.SshRunner(root, popen_factory=fail_setup)
            with self.assertRaises(transition.SshSetupError):
                runner.receive_file(
                    "nimo-1", "/absolute", root / "partial",
                    expected_size=1, operation="evidence")
            record = json.loads(runner.evidence_path.read_text().splitlines()[-1])
            self.assertEqual(record["failure_class"], "process-setup")
            self.assertEqual(record["pid"], 0)

    def test_receive_file_retains_local_sink_failure(self):
        payload = b"x"
        with tempfile.TemporaryDirectory() as temporary:
            root = Path(temporary).resolve()

            def factory(_command, **kwargs):
                return subprocess.Popen(
                    [sys.executable, "-c", "import os; os.write(1,b'x')"],
                    **kwargs)

            runner = transition.SshRunner(root, popen_factory=factory)
            original_fsync = transition.os.fsync
            calls = 0

            def fail_first_fsync(fd):
                nonlocal calls
                calls += 1
                if calls == 1:
                    raise OSError("injected sink")
                return original_fsync(fd)

            with mock.patch.object(transition.os, "fsync", fail_first_fsync):
                with self.assertRaisesRegex(OSError, "local evidence sink"):
                    runner.receive_file(
                        "nimo-1", "/absolute", root / "partial",
                        expected_size=len(payload), operation="evidence")
            record = json.loads(runner.evidence_path.read_text().splitlines()[-1])
            self.assertEqual(record["failure_class"], "local-evidence-sink")


if __name__ == "__main__":
    unittest.main()
