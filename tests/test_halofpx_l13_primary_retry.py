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
            SimpleNamespace(stdout="", stderr="", returncode=0),
            SimpleNamespace(stdout="ActiveState=inactive\nSubState=dead\nMainPID=0\n", stderr="", returncode=0),
            SimpleNamespace(stdout="", stderr="", returncode=0),
        ))
        with mock.patch.object(retry, "ssh", side_effect=lambda *args, **kwargs: next(responses)):
            retry.stop_worker("fixture")


if __name__ == "__main__":
    unittest.main()
