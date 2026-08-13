from __future__ import annotations

import ast
import contextlib
import dataclasses
import hashlib
import importlib.util
import inspect
import io
import json
import sys
import tempfile
import unittest
from pathlib import Path
from typing import Any


REPO = Path(__file__).resolve().parents[1]
SOURCE = REPO / "scripts" / "halofpx_strix_recovery_watchdog.py"
SPEC = importlib.util.spec_from_file_location("halofpx_strix_recovery_watchdog", SOURCE)
assert SPEC is not None and SPEC.loader is not None
watchdog = importlib.util.module_from_spec(SPEC)
sys.modules[SPEC.name] = watchdog
SPEC.loader.exec_module(watchdog)


def _identity(host: str, role: str, unit: str, pid: int, marker: str, start: int) -> Any:
    return watchdog.ServiceIdentity(
        host=host,
        role=role,
        unit=unit,
        pid=pid,
        invocation_id=marker * 32,
        restart_count=4,
        process_start_monotonic_ns=start,
        active_enter_monotonic_ns=start + 100,
        boot_id=("a" if host == watchdog.COORDINATOR_HOST else "b") * 32,
    )


def make_authority() -> Any:
    targets = tuple(sorted((
        watchdog.DisposableTarget(
            watchdog.COORDINATOR_HOST, "path", "/var/tmp/halofpx-watchdog-coordinator-txn-001",
        ),
        watchdog.DisposableTarget(watchdog.COORDINATOR_HOST, "port", "18080"),
        watchdog.DisposableTarget(
            watchdog.COORDINATOR_HOST, "unit", "halofpx-watchdog-coordinator.service",
        ),
        watchdog.DisposableTarget(
            watchdog.WORKER_HOST, "path", "/var/tmp/halofpx-watchdog-worker-txn-001",
        ),
        watchdog.DisposableTarget(watchdog.WORKER_HOST, "port", "50252"),
        watchdog.DisposableTarget(
            watchdog.WORKER_HOST, "unit", "halofpx-watchdog-worker.service",
        ),
    )))
    value = watchdog.PreverifiedAuthority(
        schema=watchdog.AUTHORITY_SCHEMA,
        transaction_id="txn-001",
        nonce_sha256="c" * 64,
        source_commit="d" * 40,
        executable_sha256="e" * 64,
        verification_receipt_sha256="f" * 64,
        maintenance_lease_ns=5_000_000,
        recovery_timeout_ns=5_000_000,
        coordinator_before=_identity(
            watchdog.COORDINATOR_HOST, "coordinator", watchdog.COORDINATOR_UNIT,
            1001, "1", 10_000,
        ),
        worker_before=_identity(
            watchdog.WORKER_HOST, "worker", watchdog.WORKER_UNIT,
            2001, "2", 20_000,
        ),
        disposable_allowlist=targets,
        authority_sha256="0" * 64,
    )
    return dataclasses.replace(
        value,
        authority_sha256=watchdog.sha256_bytes(watchdog.canonical_bytes(value.payload())),
    )


def read_json(path: Path) -> dict[str, Any]:
    return json.loads(path.read_text(encoding="utf-8"))


def write_json(path: Path, value: dict[str, Any]) -> None:
    path.write_bytes(watchdog.canonical_bytes(value))


def events(root: Path, host: str) -> list[dict[str, Any]]:
    return [read_json(path) for path in sorted((root / host).glob("event-*.json"))]


def terminal(root: Path, host: str) -> dict[str, Any]:
    return read_json(root / host / "local-terminal.json")


def rebuild_unsigned_custody(root: Path) -> None:
    """Rebuild all non-authoritative hashes after an adversarial semantic edit."""

    for host in watchdog.HOST_ROLE:
        node_root = root / host
        members = {
            path.name: hashlib.sha256(path.read_bytes()).hexdigest()
            for path in sorted(node_root.iterdir(), key=lambda item: item.name)
            if path.is_file() and not path.name.startswith(".") and path.name not in {
                "LOCAL-SHA256SUMS.json", "LOCAL-FINALIZED.json", "pair-ack.json",
                "PAIR-FINALIZED.json",
            }
        }
        write_json(node_root / "LOCAL-SHA256SUMS.json", {
            "schema": watchdog.MANIFEST_SCHEMA,
            "host": host,
            "role": watchdog.HOST_ROLE[host],
            "files": members,
        })
        previous_marker = read_json(node_root / "LOCAL-FINALIZED.json")
        write_json(node_root / "LOCAL-FINALIZED.json", {
            "schema": watchdog.LOCAL_FINAL_SCHEMA,
            "host": host,
            "role": watchdog.HOST_ROLE[host],
            "terminal_sha256": hashlib.sha256(
                (node_root / "local-terminal.json").read_bytes(),
            ).hexdigest(),
            "manifest_sha256": hashlib.sha256(
                (node_root / "LOCAL-SHA256SUMS.json").read_bytes(),
            ).hexdigest(),
            "publication_durability": previous_marker["publication_durability"],
        })

    ack = read_json(root / watchdog.COORDINATOR_HOST / "pair-ack.json")
    ack["terminal_sha256_by_host"] = {
        host: hashlib.sha256((root / host / "local-terminal.json").read_bytes()).hexdigest()
        for host in watchdog.HOST_ROLE
    }
    ack["local_marker_sha256_by_host"] = {
        host: hashlib.sha256((root / host / "LOCAL-FINALIZED.json").read_bytes()).hexdigest()
        for host in watchdog.HOST_ROLE
    }
    binding = {
        "authority_sha256": ack["authority_sha256"],
        "verification_receipt_sha256": ack["verification_receipt_sha256"],
        "transaction_id": ack["transaction_id"],
        "terminal_sha256_by_host": ack["terminal_sha256_by_host"],
        "local_marker_sha256_by_host": ack["local_marker_sha256_by_host"],
    }
    ack["pair_binding_sha256"] = watchdog.sha256_bytes(watchdog.canonical_bytes(binding))
    for host in watchdog.HOST_ROLE:
        node_root = root / host
        write_json(node_root / "pair-ack.json", ack)
        old_marker = read_json(node_root / "PAIR-FINALIZED.json")
        write_json(node_root / "PAIR-FINALIZED.json", {
            "schema": watchdog.PAIR_FINAL_SCHEMA,
            "host": host,
            "pair_ack_sha256": hashlib.sha256(
                (node_root / "pair-ack.json").read_bytes(),
            ).hexdigest(),
            "publication_durability": old_marker["publication_durability"],
        })


class OfflineRecoveryWatchdogTests(unittest.TestCase):
    def setUp(self) -> None:
        self._temporary = tempfile.TemporaryDirectory()
        self.addCleanup(self._temporary.cleanup)
        self.base = Path(self._temporary.name)
        self.authority = make_authority()
        self._counter = 0

    def run_model(
            self,
            loss_point: Any = None,
            faults: Any = None,
    ) -> tuple[Path, Any]:
        self._counter += 1
        root = self.base / f"evidence-{self._counter:03d}"
        result = watchdog.run_offline_model(
            self.authority,
            root,
            loss_point or watchdog.ControllerLossPoint.DURING_EXPERIMENT,
            faults or watchdog.ModelFaults(),
        )
        return root, result

    def verify(self, root: Path) -> dict[str, Any]:
        return watchdog.verify_paired_bundle(
            root, expected_authority_sha256=self.authority.authority_sha256,
        )

    def test_source_is_literal_offline_only_without_runner_or_transport(self) -> None:
        tree = ast.parse(SOURCE.read_text(encoding="utf-8"))
        imported = {
            alias.name.split(".")[0]
            for node in ast.walk(tree)
            if isinstance(node, (ast.Import, ast.ImportFrom))
            for alias in node.names
        }
        self.assertTrue({"subprocess", "socket", "paramiko", "fabric", "requests"}.isdisjoint(imported))
        self.assertIs(watchdog.TARGET_EXECUTION_ENABLED, False)
        self.assertIs(watchdog.OFFLINE_FAKE_ONLY, True)
        self.assertFalse(hasattr(watchdog, "Runner"))
        self.assertNotIn("runner", inspect.signature(watchdog.run_offline_model).parameters)
        with contextlib.redirect_stderr(io.StringIO()), self.assertRaises(SystemExit):
            watchdog.build_parser().parse_args(["run-target"])

    def test_all_controller_loss_points_recover_production_not_experiment(self) -> None:
        for loss_point in watchdog.ControllerLossPoint:
            with self.subTest(loss_point=loss_point.value):
                root, result = self.run_model(loss_point)
                self.assertEqual(result.status, "success")
                self.assertTrue(result.recovery_complete)
                self.assertTrue(result.paired)
                self.assertFalse(result.experiment_continuation_allowed)
                ack = self.verify(root)
                self.assertEqual(ack["status"], "success")
                self.assertIs(ack["experiment_continuation_allowed"], False)

    def test_same_closed_input_produces_byte_identical_evidence_tree(self) -> None:
        first, _ = self.run_model()
        second, _ = self.run_model()
        first_files = {
            path.relative_to(first).as_posix(): path.read_bytes()
            for path in first.rglob("*") if path.is_file()
        }
        second_files = {
            path.relative_to(second).as_posix(): path.read_bytes()
            for path in second.rglob("*") if path.is_file()
        }
        self.assertEqual(first_files, second_files)

    def test_controller_deadline_at_each_loss_point_still_chooses_recovery(self) -> None:
        for loss_point in watchdog.ControllerLossPoint:
            with self.subTest(loss_point=loss_point.value):
                root, result = self.run_model(
                    loss_point,
                    watchdog.ModelFaults(controller_deadline_phase=loss_point),
                )
                self.assertTrue(result.recovery_complete)
                self.assertEqual(self.verify(root)["status"], "success")
                for host in watchdog.HOST_ROLE:
                    trigger = events(root, host)[2]
                    self.assertEqual(trigger["detail"], "maintenance-deadline-expired")
                    record = terminal(root, host)
                    if record["observed_absent"]:
                        action = next(
                            item for item in events(root, host)
                            if item["phase"].endswith("service-recovery-actuation")
                        )
                        self.assertGreater(
                            record["final_observation"]["service"]["process_start_monotonic_ns"],
                            action["monotonic_ns"],
                        )

    def test_recovery_deadline_refuses_at_every_node_phase(self) -> None:
        for phase in watchdog.RECOVERY_PHASES:
            with self.subTest(phase=phase):
                root, result = self.run_model(
                    faults=watchdog.ModelFaults(recovery_deadline_phase=phase),
                )
                self.assertTrue(result.paired)
                self.assertEqual(result.status, "failure")
                self.assertFalse(result.recovery_complete)
                self.assertEqual(self.verify(root)["status"], "failure")
                host = watchdog.ROLE_HOST[phase.split(":", 1)[0]]
                event = next(item for item in events(root, host) if item["phase"] == phase)
                self.assertEqual(event["outcome"], "failed")
                later = events(root, host)[events(root, host).index(event) + 1:-1]
                self.assertTrue(all(item["outcome"] == "skipped" for item in later))

    def test_worker_ready_bytes_are_durable_before_coordinator_action(self) -> None:
        root, _ = self.run_model()
        worker_ready = (root / watchdog.WORKER_HOST / "ready-receipt.json").read_bytes()
        expected_digest = hashlib.sha256(worker_ready).hexdigest()
        coordinator = terminal(root, watchdog.COORDINATOR_HOST)
        self.assertEqual(coordinator["worker_ready_receipt_sha256"], expected_digest)
        coordinator_events = events(root, watchdog.COORDINATOR_HOST)
        peer_index = next(
            index for index, value in enumerate(coordinator_events)
            if value["phase"] == "coordinator:peer-ready-receipt"
        )
        action_index = next(
            index for index, value in enumerate(coordinator_events)
            if value["phase"] == "coordinator:service-recovery-actuation"
        )
        self.assertLess(peer_index, action_index)
        self.assertEqual(coordinator_events[peer_index]["detail"], f"sha256={expected_digest}")

    def test_identity_reconciliation_preserves_or_requires_exact_freshness(self) -> None:
        cases = {
            watchdog.ControllerLossPoint.BEFORE_FIRST_STOP: (False, False),
            watchdog.ControllerLossPoint.AFTER_COORDINATOR_STOP: (True, False),
            watchdog.ControllerLossPoint.AFTER_BOTH_STOPS: (True, True),
            watchdog.ControllerLossPoint.DURING_EXPERIMENT: (True, True),
        }
        for loss_point, (coordinator_fresh, worker_fresh) in cases.items():
            with self.subTest(loss_point=loss_point.value):
                root, _ = self.run_model(loss_point)
                for host, fresh in (
                    (watchdog.COORDINATOR_HOST, coordinator_fresh),
                    (watchdog.WORKER_HOST, worker_fresh),
                ):
                    record = terminal(root, host)
                    before = record["before_identity"]
                    after = record["final_observation"]["service"]
                    if fresh:
                        self.assertNotEqual(after["pid"], before["pid"])
                        self.assertNotEqual(after["invocation_id"], before["invocation_id"])
                        self.assertGreater(after["process_start_monotonic_ns"], before["process_start_monotonic_ns"])
                        self.assertGreater(after["active_enter_monotonic_ns"], before["active_enter_monotonic_ns"])
                    else:
                        self.assertEqual(after, before)
                    self.assertEqual(after["restart_count"], before["restart_count"])
                    self.assertEqual(after["boot_id"], before["boot_id"])

    def test_disposable_cleanup_is_exact_closed_world_and_allowlisted(self) -> None:
        root, result = self.run_model()
        self.assertTrue(result.recovery_complete)
        for host in watchdog.HOST_ROLE:
            record = terminal(root, host)
            self.assertTrue(record["cleanup_complete"])
            self.assertTrue(record["final_observation"]["disposable_scan_complete"])
            self.assertEqual(record["final_observation"]["disposables"], [])
            cleanup = next(
                item for item in events(root, host)
                if item["phase"].endswith(":cleanup-observation")
            )
            self.assertEqual(cleanup["outcome"], "accepted")

    def test_peer_unreachable_recovers_worker_and_refuses_coordinator(self) -> None:
        root, result = self.run_model(
            watchdog.ControllerLossPoint.AFTER_BOTH_STOPS,
            watchdog.ModelFaults(peer_unreachable=True),
        )
        self.assertFalse(result.paired)
        self.assertFalse(result.recovery_complete)
        self.assertTrue(terminal(root, watchdog.WORKER_HOST)["recovery_ready"])
        coordinator = terminal(root, watchdog.COORDINATOR_HOST)
        self.assertFalse(coordinator["recovery_ready"])
        self.assertIsNone(coordinator["final_observation"]["service"])
        action = next(
            item for item in events(root, watchdog.COORDINATOR_HOST)
            if item["phase"] == "coordinator:service-recovery-actuation"
        )
        self.assertEqual(action["outcome"], "skipped")
        for host in watchdog.HOST_ROLE:
            self.assertFalse((root / host / "pair-ack.json").exists())

    def test_local_reboot_or_boot_id_drift_refuses_recovery(self) -> None:
        for host in watchdog.HOST_ROLE:
            with self.subTest(host=host):
                root, result = self.run_model(faults=watchdog.ModelFaults(reboot_host=host))
                self.assertTrue(result.paired)
                self.assertFalse(result.recovery_complete)
                self.assertEqual(self.verify(root)["status"], "failure")
                boot = next(item for item in events(root, host) if item["phase"].endswith("boot-reconcile"))
                self.assertEqual(boot["outcome"], "failed")

    def test_cleanup_lost_response_uses_postcondition_but_cannot_claim_success(self) -> None:
        for host in watchdog.HOST_ROLE:
            with self.subTest(host=host):
                root, result = self.run_model(
                    faults=watchdog.ModelFaults(cleanup_lost_response_host=host),
                )
                self.assertTrue(result.paired)
                self.assertTrue(result.recovery_complete)
                self.assertEqual(result.status, "failure")
                self.assertEqual(self.verify(root)["status"], "failure")
                host_events = events(root, host)
                actuation = next(item for item in host_events if item["phase"].endswith("cleanup-actuation"))
                observation = next(item for item in host_events if item["phase"].endswith("cleanup-observation"))
                self.assertEqual(actuation["outcome"], "lost-response")
                self.assertEqual(observation["outcome"], "accepted")

    def test_cleanup_lost_response_without_effect_refuses_on_residue(self) -> None:
        host = watchdog.WORKER_HOST
        root, result = self.run_model(faults=watchdog.ModelFaults(
            cleanup_lost_response_host=host,
            cleanup_no_effect_host=host,
        ))
        self.assertFalse(result.recovery_complete)
        self.assertEqual(self.verify(root)["status"], "failure")
        observation = next(
            item for item in events(root, host) if item["phase"].endswith("cleanup-observation")
        )
        self.assertEqual(observation["outcome"], "failed")

    def test_worker_active_but_not_application_ready_blocks_coordinator(self) -> None:
        root, result = self.run_model(
            watchdog.ControllerLossPoint.AFTER_BOTH_STOPS,
            watchdog.ModelFaults(worker_readiness_false=True),
        )
        self.assertFalse(result.recovery_complete)
        worker = terminal(root, watchdog.WORKER_HOST)
        self.assertIsNotNone(worker["final_observation"]["service"])
        self.assertFalse(worker["final_observation"]["application_ready"])
        self.assertFalse(worker["recovery_ready"])
        coordinator = terminal(root, watchdog.COORDINATOR_HOST)
        self.assertIsNone(coordinator["final_observation"]["service"])
        self.assertEqual(self.verify(root)["status"], "failure")

    def test_coordinator_ambiguous_start_requires_independent_postcondition(self) -> None:
        root, result = self.run_model(
            watchdog.ControllerLossPoint.AFTER_COORDINATOR_STOP,
            watchdog.ModelFaults(coordinator_start_lost_response=True),
        )
        self.assertTrue(result.recovery_complete)
        self.assertEqual(result.status, "failure")
        coordinator = terminal(root, watchdog.COORDINATOR_HOST)
        self.assertTrue(coordinator["recovery_ready"])
        host_events = events(root, watchdog.COORDINATOR_HOST)
        start = next(item for item in host_events if item["phase"].endswith("service-recovery-actuation"))
        postcondition = next(item for item in host_events if item["phase"].endswith("service-postcondition"))
        self.assertEqual(start["outcome"], "lost-response")
        self.assertEqual(postcondition["outcome"], "accepted")
        self.assertEqual(self.verify(root)["status"], "failure")

    def test_stale_identity_reappearance_is_refused_on_each_host(self) -> None:
        for host in watchdog.HOST_ROLE:
            with self.subTest(host=host):
                root, result = self.run_model(
                    watchdog.ControllerLossPoint.AFTER_BOTH_STOPS,
                    watchdog.ModelFaults(stale_reappearance_host=host),
                )
                self.assertFalse(result.recovery_complete)
                service = next(
                    item for item in events(root, host) if item["phase"].endswith("service-observation")
                )
                self.assertEqual(service["outcome"], "failed")
                self.assertIn("stale", service["detail"])
                self.assertEqual(self.verify(root)["status"], "failure")

    def test_restart_count_drift_is_refused_for_preserved_and_fresh_services(self) -> None:
        for host in watchdog.HOST_ROLE:
            for loss_point in (
                    watchdog.ControllerLossPoint.BEFORE_FIRST_STOP,
                    watchdog.ControllerLossPoint.AFTER_BOTH_STOPS):
                with self.subTest(host=host, loss_point=loss_point.value):
                    root, result = self.run_model(
                        loss_point, watchdog.ModelFaults(restart_drift_host=host),
                    )
                    self.assertFalse(result.recovery_complete)
                    self.assertEqual(self.verify(root)["status"], "failure")
                    record = terminal(root, host)
                    self.assertNotEqual(
                        record["final_observation"]["service"]["restart_count"],
                        record["before_identity"]["restart_count"],
                    )

    def test_hmm_census_failures_are_closed_world_refusals(self) -> None:
        cases = (
            {"foreign_hmm_owner_host": watchdog.WORKER_HOST},
            {"incomplete_hmm_census_host": watchdog.WORKER_HOST},
            {"non_elevated_hmm_census_host": watchdog.WORKER_HOST},
            {"foreign_hmm_owner_host": watchdog.COORDINATOR_HOST},
        )
        for values in cases:
            with self.subTest(values=values):
                root, result = self.run_model(faults=watchdog.ModelFaults(**values))
                self.assertFalse(result.recovery_complete)
                self.assertEqual(self.verify(root)["status"], "failure")

    def test_kernel_counter_delta_is_refused_on_each_host(self) -> None:
        for host in watchdog.HOST_ROLE:
            with self.subTest(host=host):
                root, result = self.run_model(faults=watchdog.ModelFaults(kernel_delta_host=host))
                self.assertFalse(result.recovery_complete)
                reconcile = next(
                    item for item in events(root, host) if item["phase"].endswith("kernel-reconcile")
                )
                self.assertEqual(reconcile["outcome"], "failed")
                self.assertEqual(self.verify(root)["status"], "failure")

    def test_monotonic_clock_regression_is_refused_on_each_host(self) -> None:
        for host in watchdog.HOST_ROLE:
            with self.subTest(host=host):
                root, result = self.run_model(
                    faults=watchdog.ModelFaults(monotonic_regression_host=host),
                )
                self.assertFalse(result.recovery_complete)
                self.assertEqual(self.verify(root)["status"], "failure")

    def test_unknown_disposable_or_incomplete_scan_is_refused(self) -> None:
        for field in ("unknown_disposable_host", "incomplete_disposable_scan_host"):
            for host in watchdog.HOST_ROLE:
                with self.subTest(field=field, host=host):
                    root, result = self.run_model(faults=watchdog.ModelFaults(**{field: host}))
                    self.assertFalse(result.recovery_complete)
                    self.assertEqual(self.verify(root)["status"], "failure")

    def test_tampered_worker_ready_delivery_blocks_coordinator_start(self) -> None:
        root, result = self.run_model(
            watchdog.ControllerLossPoint.AFTER_BOTH_STOPS,
            watchdog.ModelFaults(tamper_worker_ready_receipt=True),
        )
        self.assertFalse(result.recovery_complete)
        coordinator = terminal(root, watchdog.COORDINATOR_HOST)
        self.assertIsNone(coordinator["final_observation"]["service"])
        self.assertEqual(self.verify(root)["status"], "failure")

    def test_expected_authority_digest_is_an_external_required_anchor(self) -> None:
        root, _ = self.run_model()
        with self.assertRaisesRegex(watchdog.WatchdogError, "externally expected"):
            watchdog.verify_paired_bundle(root, expected_authority_sha256="0" * 64)
        with self.assertRaises(TypeError):
            watchdog.verify_paired_bundle(root)

    def test_authority_hash_and_exact_allowlist_are_fail_closed(self) -> None:
        with self.assertRaisesRegex(watchdog.WatchdogError, "does not bind"):
            dataclasses.replace(self.authority, source_commit="0" * 40).validate()
        with self.assertRaisesRegex(watchdog.WatchdogError, "canonical order"):
            reversed_targets = tuple(reversed(self.authority.disposable_allowlist))
            value = dataclasses.replace(
                self.authority, disposable_allowlist=reversed_targets, authority_sha256="0" * 64,
            )
            value = dataclasses.replace(
                value, authority_sha256=watchdog.sha256_bytes(watchdog.canonical_bytes(value.payload())),
            )
            value.validate()
        unsafe = watchdog.DisposableTarget(watchdog.WORKER_HOST, "port", "50052")
        with self.assertRaisesRegex(watchdog.WatchdogError, "unsafe or protected"):
            unsafe.validate("test")
        unsafe_path = watchdog.DisposableTarget(
            watchdog.WORKER_HOST, "path", "/var/tmp/halofpx-watchdog-worker\n",
        )
        with self.assertRaisesRegex(watchdog.WatchdogError, "outside"):
            unsafe_path.validate("test")

    def test_existing_evidence_root_is_never_overwritten(self) -> None:
        root = self.base / "already-exists"
        root.mkdir()
        marker = root / "user-owned.txt"
        marker.write_text("preserve\n", encoding="utf-8")
        with self.assertRaisesRegex(watchdog.WatchdogError, "must be absent"):
            watchdog.run_offline_model(
                self.authority, root, watchdog.ControllerLossPoint.BEFORE_FIRST_STOP,
            )
        self.assertEqual(marker.read_text(encoding="utf-8"), "preserve\n")

    def test_noncanonical_and_duplicate_json_are_rejected(self) -> None:
        with self.assertRaisesRegex(watchdog.WatchdogError, "canonical"):
            watchdog.parse_canonical_json(b'{"b": 1, "a": 2}\n', "test")
        with self.assertRaisesRegex(watchdog.WatchdogError, "duplicate"):
            watchdog.parse_canonical_json(b'{"a":1,"a":2}\n', "test")
        with self.assertRaisesRegex(watchdog.WatchdogError, "non-finite"):
            watchdog.parse_canonical_json(b'{"a":NaN}\n', "test")

    def test_partial_or_corrupt_terminal_record_is_rejected(self) -> None:
        for mutation in ("truncate", "invalid-json"):
            with self.subTest(mutation=mutation):
                root, _ = self.run_model()
                path = root / watchdog.WORKER_HOST / "local-terminal.json"
                raw = path.read_bytes()
                path.write_bytes(raw[: len(raw) // 2] if mutation == "truncate" else b"not-json\n")
                with self.assertRaises(watchdog.WatchdogError):
                    self.verify(root)

    def test_semantic_terminal_tamper_fails_even_after_all_unsigned_hashes_rebuilt(self) -> None:
        root, _ = self.run_model()
        path = root / watchdog.WORKER_HOST / "local-terminal.json"
        value = read_json(path)
        value["target_execution_enabled"] = True
        write_json(path, value)
        rebuild_unsigned_custody(root)
        with self.assertRaisesRegex(watchdog.WatchdogError, "hard-off"):
            self.verify(root)

    def test_semantic_event_tamper_fails_even_after_all_unsigned_hashes_rebuilt(self) -> None:
        root, _ = self.run_model()
        path = root / watchdog.WORKER_HOST / "event-0004.json"
        value = read_json(path)
        value["phase"] = "worker:service-recovery-actuation"
        write_json(path, value)
        rebuild_unsigned_custody(root)
        with self.assertRaisesRegex(watchdog.WatchdogError, "phase"):
            self.verify(root)

    def test_peer_arm_tamper_fails_after_all_unsigned_hashes_are_rebuilt(self) -> None:
        root, _ = self.run_model()
        host = watchdog.WORKER_HOST
        peer_path = root / host / "peer-arm.json"
        peer = read_json(peer_path)
        peer["peer_arm_sha256"] = "0" * 64
        write_json(peer_path, peer)
        event_path = root / host / "event-0002.json"
        event = read_json(event_path)
        event["detail"] = f"peer_arm_sha256={'0' * 64}"
        write_json(event_path, event)
        rebuild_unsigned_custody(root)
        with self.assertRaisesRegex(watchdog.WatchdogError, "cross-bind"):
            self.verify(root)

    def test_pair_acknowledgement_partial_divergent_or_continuation_tamper_is_rejected(self) -> None:
        root, _ = self.run_model()
        worker_ack = root / watchdog.WORKER_HOST / "pair-ack.json"
        worker_ack.write_bytes(worker_ack.read_bytes()[:50])
        with self.assertRaises(watchdog.WatchdogError):
            self.verify(root)

        root, _ = self.run_model()
        for host in watchdog.HOST_ROLE:
            path = root / host / "pair-ack.json"
            value = read_json(path)
            value["experiment_continuation_allowed"] = True
            write_json(path, value)
            marker_path = root / host / "PAIR-FINALIZED.json"
            marker = read_json(marker_path)
            marker["pair_ack_sha256"] = hashlib.sha256(path.read_bytes()).hexdigest()
            write_json(marker_path, marker)
        with self.assertRaisesRegex(watchdog.WatchdogError, "hard-off"):
            self.verify(root)

    def test_closed_inventory_rejects_unlisted_file(self) -> None:
        root, _ = self.run_model()
        (root / watchdog.WORKER_HOST / "surprise.txt").write_text("not admitted\n", encoding="utf-8")
        with self.assertRaisesRegex(watchdog.WatchdogError, "inventory"):
            self.verify(root)

    def test_cli_verifier_requires_and_uses_external_authority_digest(self) -> None:
        root, _ = self.run_model()
        output = io.StringIO()
        with contextlib.redirect_stdout(output):
            status = watchdog.main([
                "verify-pair", str(root),
                "--expected-authority-sha256", self.authority.authority_sha256,
            ])
        self.assertEqual(status, 0)
        self.assertEqual(json.loads(output.getvalue())["status"], "success")


if __name__ == "__main__":
    unittest.main()
