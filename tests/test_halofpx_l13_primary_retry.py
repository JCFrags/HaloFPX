import importlib.util
import sys
import unittest
from pathlib import Path
from types import SimpleNamespace
from unittest import mock


SCRIPT = Path(__file__).parents[1] / "scripts" / "halofpx-l13-primary-retry.py"
SPEC = importlib.util.spec_from_file_location("halofpx_primary_retry", SCRIPT)
assert SPEC and SPEC.loader
retry = importlib.util.module_from_spec(SPEC)
sys.modules[SPEC.name] = retry
SPEC.loader.exec_module(retry)


class PrimaryRetryTests(unittest.TestCase):
    def test_start_worker_requires_admitted_caps_before_return(self):
        responses = iter((
            SimpleNamespace(stdout="", stderr="", returncode=0),
            SimpleNamespace(stdout='{"admitted": true, "endpoint": "10.44.0.1:50176"}\n', stderr="", returncode=0),
            SimpleNamespace(stdout="active\n", stderr="", returncode=0),
            SimpleNamespace(stdout="42\n", stderr="", returncode=0),
            SimpleNamespace(stdout='LISTEN 0 1 10.44.0.1:50176 0.0.0.0:* users:(("rpc",pid=42,fd=3))\n', stderr="", returncode=0),
        ))
        calls = []
        with mock.patch.object(
            retry, "ssh", side_effect=lambda *args, **kwargs: calls.append(args) or next(responses)
        ):
            pid, evidence = retry.start_worker(True, "fixture")
            self.assertEqual(pid, 42)
            self.assertTrue(evidence["admitted"])
        self.assertTrue(any(str(value).endswith("halofpx_rpc_readiness.py") for value in calls[1]))

    def test_start_worker_accepts_only_confirmed_feature_off_protocol(self):
        responses = iter((
            SimpleNamespace(stdout="", stderr="", returncode=0),
            SimpleNamespace(stdout='{"admitted": false, "feature_off_confirmed": true, "endpoint": "10.44.0.1:50176"}\n', stderr="", returncode=0),
            SimpleNamespace(stdout="active\n", stderr="", returncode=0),
            SimpleNamespace(stdout="42\n", stderr="", returncode=0),
            SimpleNamespace(stdout='LISTEN 0 1 10.44.0.1:50176 0.0.0.0:* users:(("rpc",pid=42,fd=3))\n', stderr="", returncode=0),
        ))
        with mock.patch.object(retry, "ssh", side_effect=lambda *args, **kwargs: next(responses)):
            pid, evidence = retry.start_worker(False, "fixture")
            self.assertEqual(pid, 42)
            self.assertTrue(evidence["feature_off_confirmed"])

    def test_listener_pid_requires_exact_port(self):
        text = 'LISTEN 0 1 0.0.0.0:501760 0.0.0.0:* users:(("rpc",pid=12,fd=3))\n'
        self.assertEqual(retry.listener_pid(text, 50176), 0)

    def test_complete_state_windows_include_full_operations(self):
        capture = "\n".join((
            "normal inference",
            "[alloc_buffer] device: 0, size: 100 -> remote_ptr: 1",
            "[copy_tensor] one",
            "[copy_tensor] two",
            "[halofpx-state] stored rank=1 components=2 bytes=100",
            "later inference",
        ))
        restore = "\n".join((
            "normal inference",
            "[alloc_buffer] device: 0, size: 100 -> remote_ptr: 2",
            "[buffer_get_base]",
            "[halofpx-state] ready rank=1 components=2 bytes=100",
            "[halofpx-state] apply rank=1 components=2 bytes=100",
            "[copy_tensor] one",
            "[copy_tensor] two",
            "[free_buffer] remote_ptr: 2",
            "later inference",
        ))
        capture_window, restore_window = retry.state_windows(capture, restore)
        self.assertEqual(capture_window[0].split()[0], "[alloc_buffer]")
        self.assertEqual(capture_window[-1].split()[0], "[halofpx-state]")
        self.assertEqual(restore_window[0].split()[0], "[alloc_buffer]")
        self.assertEqual(restore_window[-1].split()[0], "[free_buffer]")

    def test_get_set_anywhere_in_state_window_rejects(self):
        capture = "\n".join((
            "[alloc_buffer] size: 100",
            "[get_tensor] forbidden",
            "[halofpx-state] stored bytes=100",
        ))
        restore = "\n".join((
            "[alloc_buffer] size: 100",
            "[halofpx-state] ready bytes=100",
            "[halofpx-state] apply bytes=100",
            "[free_buffer]",
        ))
        with self.assertRaises(retry.RetryError):
            retry.state_windows(capture, restore)

    def test_stop_worker_requires_inactive_dead_zero_and_closed_port(self):
        responses = iter((
            SimpleNamespace(stdout="LoadState=not-found\nActiveState=inactive\nSubState=dead\nMainPID=0\n", stderr="", returncode=0),
            SimpleNamespace(stdout="", stderr="", returncode=0),
        ))
        calls = []
        with mock.patch.object(
            retry, "ssh", side_effect=lambda *args, **kwargs: calls.append(args) or next(responses)
        ):
            retry.stop_worker("fixture")
        self.assertFalse(any("stop" in call for call in calls))


if __name__ == "__main__":
    unittest.main()
