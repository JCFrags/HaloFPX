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
    def test_consumes_preprovisioned_identical_keys_without_secret_argv(self):
        digest = "a" * 64
        responses = iter((
            SimpleNamespace(stdout="regular file:connorb:600:130\n", stderr="", returncode=0),
            SimpleNamespace(stdout=f"{digest}  key\n", stderr="", returncode=0),
            SimpleNamespace(stdout="regular file:connorb:600:130\n", stderr="", returncode=0),
            SimpleNamespace(stdout=f"{digest}  key\n", stderr="", returncode=0),
        ))
        calls = []
        with mock.patch.dict("os.environ", {retry.CHANNEL_KEY_DIGEST_ENV: digest}, clear=False), mock.patch.object(
            retry, "ssh", side_effect=lambda *args, **kwargs: calls.append(args) or next(responses)
        ):
            self.assertEqual(retry.validate_provisioned_keys(), digest)
        rendered = repr(calls)
        self.assertNotIn("install", rendered)
        self.assertNotIn("chmod", rendered)
        self.assertNotIn("openssl", rendered)

    def test_changed_preprovisioned_key_fails_closed(self):
        expected = "a" * 64
        responses = iter((
            SimpleNamespace(stdout="regular file:connorb:600:130\n", stderr="", returncode=0),
            SimpleNamespace(stdout=f"{'b' * 64}  key\n", stderr="", returncode=0),
        ))
        with mock.patch.dict("os.environ", {retry.CHANNEL_KEY_DIGEST_ENV: expected}, clear=False), mock.patch.object(
            retry, "ssh", side_effect=lambda *args, **kwargs: next(responses)
        ):
            with self.assertRaises(retry.CanaryError):
                retry.validate_provisioned_keys()

    def test_l16_primary_invocation_is_exactly_frozen(self):
        completed = SimpleNamespace(stdout="mode=cold\n", stderr="", returncode=0)
        with mock.patch.object(retry, "ssh", return_value=completed) as remote:
            retry.canary("cold", "cold")
        command = list(remote.call_args.args[1:])
        self.assertIn("--unit=halofpx-l16-primary-canary-cold-20260721", command)
        self.assertIn("--wait", command)
        self.assertIn("--collect", command)
        self.assertEqual(
            retry.CANARY_BIN,
            "/var/tmp/halofpx-l17-src-nimo2/build-l17/bin/test-halofpx-distributed-state-canary",
        )
        expected_pairs = {
            "--hfx-expected-prompt-tokens": "1129",
            "--device": "RPC0,ROCm0",
            "--split-mode": "layer",
            "--tensor-split": "1,1",
            "--ctx-size": "4096",
            "--batch-size": "512",
            "--ubatch-size": "512",
            "--cache-type-k": "q8_0",
            "--cache-type-v": "q8_0",
            "--n-predict": "128",
            "--seed": "1234",
            "--temp": "0",
        }
        for option, value in expected_pairs.items():
            self.assertEqual(command[command.index(option) + 1], value)
        self.assertEqual(command[command.index("--file") + 1], retry.PROMPT)
        self.assertEqual(command[command.index("--model") + 1], retry.MODEL)
        self.assertEqual(command[command.index("--rpc") + 1], "10.44.0.1:50180")

    def test_start_worker_requires_admitted_caps_before_return(self):
        responses = iter((
            SimpleNamespace(stdout="", stderr="", returncode=0),
            SimpleNamespace(stdout='{"admitted": true, "endpoint": "10.44.0.1:50180"}\n', stderr="", returncode=0),
            SimpleNamespace(stdout='{"admitted": true, "endpoint": "10.44.0.1:50180"}\n', stderr="", returncode=0),
            SimpleNamespace(stdout="active\n", stderr="", returncode=0),
            SimpleNamespace(stdout="42\n", stderr="", returncode=0),
            SimpleNamespace(stdout="0123456789abcdef0123456789abcdef\n", stderr="", returncode=0),
            SimpleNamespace(stdout='LISTEN 0 1 10.44.0.1:50180 0.0.0.0:* users:(("rpc",pid=42,fd=3))\n', stderr="", returncode=0),
        ))
        calls = []
        with mock.patch.object(
            retry, "ssh", side_effect=lambda *args, **kwargs: calls.append(args) or next(responses)
        ):
            pid, invocation, evidence = retry.start_worker(True, "fixture")
            self.assertEqual(pid, 42)
            self.assertEqual(invocation, "0123456789abcdef0123456789abcdef")
            self.assertTrue(evidence["admitted"])
        self.assertTrue(any(str(value).endswith("halofpx_rpc_readiness.py") for value in calls[1]))
        self.assertIn("RPC0,ROCm0", calls[2])

    def test_start_worker_accepts_only_confirmed_feature_off_protocol(self):
        responses = iter((
            SimpleNamespace(stdout="", stderr="", returncode=0),
            SimpleNamespace(stdout='{"admitted": false, "feature_off_confirmed": true, "endpoint": "10.44.0.1:50180"}\n', stderr="", returncode=0),
            SimpleNamespace(stdout='{"admitted": true, "endpoint": "10.44.0.1:50180"}\n', stderr="", returncode=0),
            SimpleNamespace(stdout="active\n", stderr="", returncode=0),
            SimpleNamespace(stdout="42\n", stderr="", returncode=0),
            SimpleNamespace(stdout="0123456789abcdef0123456789abcdef\n", stderr="", returncode=0),
            SimpleNamespace(stdout='LISTEN 0 1 10.44.0.1:50180 0.0.0.0:* users:(("rpc",pid=42,fd=3))\n', stderr="", returncode=0),
        ))
        with mock.patch.object(retry, "ssh", side_effect=lambda *args, **kwargs: next(responses)):
            pid, invocation, evidence = retry.start_worker(False, "fixture")
            self.assertEqual(pid, 42)
            self.assertEqual(invocation, "0123456789abcdef0123456789abcdef")
            self.assertTrue(evidence["feature_off_confirmed"])

    def test_start_worker_refuses_failed_placement_before_identity_admission(self):
        responses = iter((
            SimpleNamespace(stdout="", stderr="", returncode=0),
            SimpleNamespace(stdout='{"admitted": true, "endpoint": "10.44.0.1:50180"}\n', stderr="", returncode=0),
            SimpleNamespace(stdout="", stderr="placement refusal: selected-device-order\n", returncode=3),
        ))
        calls = []
        with mock.patch.object(
            retry, "ssh", side_effect=lambda *args, **kwargs: calls.append(args) or next(responses)
        ):
            with self.assertRaises(retry.CanaryError):
                retry.start_worker(True, "fixture")
        self.assertEqual(len(calls), 3)

    def test_start_worker_refuses_malformed_placement_evidence(self):
        responses = iter((
            SimpleNamespace(stdout="", stderr="", returncode=0),
            SimpleNamespace(stdout='{"admitted": true, "endpoint": "10.44.0.1:50180"}\n', stderr="", returncode=0),
            SimpleNamespace(stdout="not-json\n", stderr="", returncode=0),
        ))
        with mock.patch.object(retry, "ssh", side_effect=lambda *args, **kwargs: next(responses)):
            with self.assertRaises(retry.CanaryError):
                retry.start_worker(True, "fixture")

    def test_positive_restore_rejects_silent_cold_fallback(self):
        fields = {
            "mode": "restore", "prompt_tokens": "1129", "saved_boundary": "1128",
            "n_batch": "512", "worker_bytes": "100", "worker_components": "1",
            "fallback": "cold", "reason": "coordinator-apply",
        }
        with self.assertRaises(retry.CanaryError):
            retry.require_result(fields, "restore", require_worker_state=True)

    def test_fault_fallbacks_require_exact_reason(self):
        fields = {
            "mode": "restore", "prompt_tokens": "1129", "saved_boundary": "1128",
            "n_batch": "512", "fallback": "cold", "reason": "worker-commit",
        }
        with self.assertRaises(retry.CanaryError):
            retry.require_result(fields, "restore", fallback_reason="worker-stage")

    def test_worker_journal_is_invocation_and_pid_bound(self):
        line = "Jul 21 host rpc-server[42]: [halofpx-state] ready bytes=100\n"
        with mock.patch.object(
            retry, "ssh", return_value=SimpleNamespace(stdout=line, stderr="", returncode=0)
        ) as remote:
            self.assertEqual(
                retry.worker_journal("fixture", "0123456789abcdef0123456789abcdef", 42), line
            )
        self.assertIn(
            "_SYSTEMD_INVOCATION_ID=0123456789abcdef0123456789abcdef",
            remote.call_args.args,
        )

    def test_worker_journal_rejects_wrong_pid(self):
        line = "Jul 21 host rpc-server[99]: [halofpx-state] ready bytes=100\n"
        with mock.patch.object(
            retry, "ssh", return_value=SimpleNamespace(stdout=line, stderr="", returncode=0)
        ):
            with self.assertRaises(retry.CanaryError):
                retry.worker_journal("fixture", "0123456789abcdef0123456789abcdef", 42)

    def test_listener_pid_requires_exact_port(self):
        text = 'LISTEN 0 1 0.0.0.0:501800 0.0.0.0:* users:(("rpc",pid=12,fd=3))\n'
        self.assertEqual(retry.listener_pid(text, 50180), 0)

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
        with self.assertRaises(retry.CanaryError):
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
