import importlib.util
import inspect
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
    def test_child_ssh_routes_through_closed_operation_classes(self):
        calls = []

        class Transport:
            def run(self, host, argv, *, operation):
                calls.append((host, argv, operation))
                return SimpleNamespace(returncode=0, stdout="ok\n", stderr="")

        prior = retry.SSH_TRANSPORT
        retry.SSH_TRANSPORT = Transport()
        try:
            retry.ssh("nimo-1", "hostname")
            retry.ssh("nimo-2", "systemd-run", "--user", "--unit=x")
        finally:
            retry.SSH_TRANSPORT = prior
        self.assertEqual(calls[0][2], "command")
        self.assertEqual(calls[1][2], "model-session")

    def test_child_session_setup_failure_uses_shared_cleanup_and_evidence(self):
        process = SimpleNamespace(pid=77, returncode=-9)
        records = []

        class Shared:
            @staticmethod
            def _create_windows_job(_process):
                raise OSError("job refused")

            @staticmethod
            def _cleanup_setup_failure(_process):
                return True, True, "taskkill_rc=0"

        module = SimpleNamespace(
            SshRunner=Shared,
            SSH_OPERATION_DEADLINES={"model-session": 1800.0},
        )
        transport = SimpleNamespace(_record=lambda value: records.append(value))
        prior_module, prior_transport = retry.SSH_TRANSPORT_MODULE, retry.SSH_TRANSPORT
        retry.SSH_TRANSPORT_MODULE, retry.SSH_TRANSPORT = module, transport
        try:
            with mock.patch.object(retry.subprocess, "Popen", return_value=process):
                with self.assertRaises(retry.CanaryError):
                    retry.start_bounded_ssh_session("nimo-2", "systemd-run x")
        finally:
            retry.SSH_TRANSPORT_MODULE, retry.SSH_TRANSPORT = prior_module, prior_transport
        self.assertEqual(records[0]["failure_class"], "process-group-setup")
        self.assertTrue(records[0]["term_sent"])
        self.assertTrue(records[0]["kill_sent"])

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

    def test_l24_primary_diagnostic_invocation_is_exactly_frozen(self):
        completed = SimpleNamespace(stdout="mode=cold\n", stderr="", returncode=0)
        with mock.patch.object(retry, "ssh", return_value=completed) as remote:
            retry.canary_sequence("residency3", "residency3")
        command = list(remote.call_args.args[1:])
        self.assertIn("--unit=halofpx-l24-primary-canary-residency3", command)
        self.assertIn("--wait", command)
        self.assertIn("--collect", command)
        self.assertEqual(
            retry.CANARY_BIN,
            "/var/tmp/halofpx-l24-source-nimo2/build-l24/bin/test-halofpx-distributed-state-canary",
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
            "--n-predict": "1",
            "--seed": "1234",
            "--temp": "0",
        }
        for option, value in expected_pairs.items():
            self.assertEqual(command[command.index(option) + 1], value)
        self.assertEqual(command[command.index("--hfx-sequence") + 1], "residency3")
        self.assertEqual(command[command.index("--file") + 1], retry.PROMPT)
        self.assertEqual(command[command.index("--model") + 1], retry.MODEL)
        self.assertEqual(command[command.index("--rpc") + 1], "10.44.0.1:50184")

    def test_start_worker_requires_admitted_caps_before_return(self):
        responses = iter((
            SimpleNamespace(stdout="", stderr="", returncode=0),
            SimpleNamespace(stdout='{"admitted": true, "endpoint": "10.44.0.1:50184"}\n', stderr="", returncode=0),
            SimpleNamespace(stdout='{"admitted": true, "endpoint": "10.44.0.1:50184"}\n', stderr="", returncode=0),
            SimpleNamespace(stdout="active\n", stderr="", returncode=0),
            SimpleNamespace(stdout="42\n", stderr="", returncode=0),
            SimpleNamespace(stdout="0123456789abcdef0123456789abcdef\n", stderr="", returncode=0),
            SimpleNamespace(stdout='LISTEN 0 1 10.44.0.1:50184 0.0.0.0:* users:(("rpc",pid=42,fd=3))\n', stderr="", returncode=0),
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
            SimpleNamespace(stdout='{"admitted": false, "feature_off_confirmed": true, "endpoint": "10.44.0.1:50184"}\n', stderr="", returncode=0),
            SimpleNamespace(stdout='{"admitted": true, "endpoint": "10.44.0.1:50184"}\n', stderr="", returncode=0),
            SimpleNamespace(stdout="active\n", stderr="", returncode=0),
            SimpleNamespace(stdout="42\n", stderr="", returncode=0),
            SimpleNamespace(stdout="0123456789abcdef0123456789abcdef\n", stderr="", returncode=0),
            SimpleNamespace(stdout='LISTEN 0 1 10.44.0.1:50184 0.0.0.0:* users:(("rpc",pid=42,fd=3))\n', stderr="", returncode=0),
        ))
        with mock.patch.object(retry, "ssh", side_effect=lambda *args, **kwargs: next(responses)):
            pid, invocation, evidence = retry.start_worker(False, "fixture")
            self.assertEqual(pid, 42)
            self.assertEqual(invocation, "0123456789abcdef0123456789abcdef")
            self.assertTrue(evidence["feature_off_confirmed"])

    def test_start_worker_refuses_failed_placement_before_identity_admission(self):
        responses = iter((
            SimpleNamespace(stdout="", stderr="", returncode=0),
            SimpleNamespace(stdout='{"admitted": true, "endpoint": "10.44.0.1:50184"}\n', stderr="", returncode=0),
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
            SimpleNamespace(stdout='{"admitted": true, "endpoint": "10.44.0.1:50184"}\n', stderr="", returncode=0),
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

    def test_diagnostic_agreement_accepts_exact_three_phase_and_receipts(self):
        digest = "a" * 64
        capture = (
            "[halofpx-state-diag] phase=capture components=64 "
            f"descriptor_content_sha256={digest}\n"
            "[halofpx-state] stored rank=1 generation=1 components=64 bytes=2454528\n"
        )
        restore = (
            "[halofpx-state-diag] phase=stage components=64 "
            f"descriptor_content_sha256={digest}\n"
            "[halofpx-state-diag] phase=apply components=64 "
            f"descriptor_content_sha256={digest}\n"
            "[halofpx-state] ready rank=1 generation=1 components=64 bytes=2454528\n"
            "[halofpx-state] apply rank=1 generation=1 status=3 components=64 bytes=2454528\n"
        )
        fields = {
            "worker_components": "64", "worker_bytes": "2454528",
            "control_sha256": "b" * 64, "local_sha256": "c" * 64,
            "component_manifest_sha256": "d" * 64,
        }
        result = retry.require_diagnostic_agreement(capture, restore, fields, dict(fields))
        self.assertEqual(result["worker_descriptor_content_sha256"], digest)

    def test_diagnostic_agreement_refuses_missing_or_malformed_phase(self):
        fields = {
            "worker_components": "64", "worker_bytes": "2454528",
            "control_sha256": "b" * 64, "local_sha256": "c" * 64,
            "component_manifest_sha256": "d" * 64,
        }
        with self.assertRaises(retry.CanaryError):
            retry.require_diagnostic_agreement(
                "[halofpx-state-diag] phase=capture components=64 "
                f"descriptor_content_sha256={'a' * 64}\n",
                "[halofpx-state-diag] phase=stage components=64 descriptor_content_sha256=bad\n",
                fields, dict(fields),
            )

    def test_diagnostic_agreement_refuses_worker_or_coordinator_mismatch(self):
        capture = (
            "[halofpx-state-diag] phase=capture components=64 "
            f"descriptor_content_sha256={'a' * 64}\n"
            "[halofpx-state] stored rank=1 generation=1 components=64 bytes=2454528\n"
        )
        restore = (
            "[halofpx-state-diag] phase=stage components=64 "
            f"descriptor_content_sha256={'a' * 64}\n"
            "[halofpx-state-diag] phase=apply components=64 "
            f"descriptor_content_sha256={'e' * 64}\n"
            "[halofpx-state] ready rank=1 generation=1 components=64 bytes=2454528\n"
            "[halofpx-state] apply rank=1 generation=1 status=3 components=64 bytes=2454528\n"
        )
        captured = {
            "worker_components": "64", "worker_bytes": "2454528",
            "control_sha256": "b" * 64, "local_sha256": "c" * 64,
            "component_manifest_sha256": "d" * 64,
        }
        with self.assertRaises(retry.CanaryError):
            retry.require_diagnostic_agreement(capture, restore, captured, dict(captured))
        restored = dict(captured)
        restored["control_sha256"] = "f" * 64
        exact_restore = restore.replace("e" * 64, "a" * 64)
        with self.assertRaises(retry.CanaryError):
            retry.require_diagnostic_agreement(capture, exact_restore, captured, restored)

    def test_diagnostic_agreement_refuses_malformed_marker_or_state_metadata(self):
        digest = "a" * 64
        fields = {
            "worker_components": "64", "worker_bytes": "2454528",
            "control_sha256": "b" * 64, "local_sha256": "c" * 64,
            "component_manifest_sha256": "d" * 64,
        }
        capture = (
            "[halofpx-state-diag] phase=capture components=64 "
            f"descriptor_content_sha256={digest} trailing\n"
            "[halofpx-state] stored rank=1 generation=1 components=64 bytes=2454528\n"
        )
        restore = (
            "[halofpx-state-diag] phase=stage components=64 "
            f"descriptor_content_sha256={digest}\n"
            "[halofpx-state-diag] phase=apply components=64 "
            f"descriptor_content_sha256={digest}\n"
            "[halofpx-state] ready rank=1 generation=1 components=64 bytes=2454528\n"
            "[halofpx-state] apply rank=1 generation=1 status=2 components=64 bytes=2454528\n"
        )
        with self.assertRaises(retry.CanaryError):
            retry.require_diagnostic_agreement(capture, restore, fields, dict(fields))

    def test_flushed_capture_evidence_requires_authenticated_complete_line(self):
        line = (
            "mode=capture label=capture "
            f"control_sha256={'a' * 64} local_sha256={'b' * 64} "
            f"component_manifest_sha256={'c' * 64} "
            "prompt_tokens=1129 saved_boundary=1128 n_batch=512 "
            "worker_bytes=2454528 worker_components=64 tokens=21549,\n"
        )
        fields = retry.require_flushed_capture_evidence(line)
        self.assertEqual(fields["tokens"], "21549,")
        for partial in (
            line.replace("control_sha256=" + "a" * 64, "control_sha256=bad"),
            line.replace("tokens=21549,", "tokens=9283,"),
            line[:-1],
        ):
            with self.assertRaises(retry.CanaryError):
                retry.require_flushed_capture_evidence(partial)

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

    def test_fresh_residency_runner_wires_validator_before_authorization(self):
        source = inspect.getsource(retry.run_diagnostic)
        self.assertLess(
            source.index("require_fresh_rpc_model_residency("),
            source.index('ssh(NIMO2, "touch", f"{RENDEZVOUS_ROOT}/restore-authorized")'))

    def test_fresh_rpc_residency_accepts_changed_worker_and_coordinator(self):
        retry.require_fresh_rpc_model_residency(
            "1" * 32, "2" * 32, 101, 202, True)

    def test_fresh_rpc_residency_rejects_unchanged_worker_epoch(self):
        with self.assertRaisesRegex(retry.CanaryError, "changed worker InvocationID"):
            retry.require_fresh_rpc_model_residency(
                "1" * 32, "1" * 32, 101, 202, True)

    def test_fresh_rpc_residency_rejects_same_coordinator_process(self):
        with self.assertRaisesRegex(retry.CanaryError, "fresh coordinator"):
            retry.require_fresh_rpc_model_residency(
                "1" * 32, "2" * 32, 101, 101, True)

    def test_fresh_rpc_residency_rejects_load_before_worker_restart(self):
        with self.assertRaisesRegex(retry.CanaryError, "load after"):
            retry.require_fresh_rpc_model_residency(
                "1" * 32, "2" * 32, 101, 202, False)

    def test_fresh_rpc_residency_rejects_wrong_stop_order(self):
        with self.assertRaisesRegex(retry.CanaryError, "coordinator must terminate"):
            retry.require_fresh_rpc_model_residency(
                "1" * 32, "2" * 32, 101, 202, True,
                capture_coordinator_stopped_before_worker=False)
        with self.assertRaisesRegex(retry.CanaryError, "worker A must stop"):
            retry.require_fresh_rpc_model_residency(
                "1" * 32, "2" * 32, 101, 202, True,
                capture_worker_stopped_before_restore=False)

    def test_fresh_rpc_residency_rejects_worker_change_after_model_load(self):
        with self.assertRaisesRegex(retry.CanaryError, "changed after restore model load"):
            retry.require_fresh_rpc_model_residency(
                "1" * 32, "2" * 32, 101, 202, True,
                restore_worker_pid=303, current_worker_pid=404,
                current_worker_invocation="2" * 32)
        with self.assertRaisesRegex(retry.CanaryError, "changed after restore model load"):
            retry.require_fresh_rpc_model_residency(
                "1" * 32, "2" * 32, 101, 202, True,
                restore_worker_pid=303, current_worker_pid=303,
                current_worker_invocation="3" * 32)


if __name__ == "__main__":
    unittest.main()
