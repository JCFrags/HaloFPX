from __future__ import annotations

import copy
import dataclasses
import datetime as dt
import hashlib
import importlib.util
import io
import json
import os
import subprocess
import sys
import tempfile
import unittest
from unittest import mock
from pathlib import Path
from typing import Any


REPO = Path(__file__).resolve().parents[1]
SOURCE = REPO / "scripts" / "halofpx_strix_maintenance.py"
EXAMPLE_AUTHORIZATION = REPO / "scripts" / "halofpx-strix-maintenance-authorization.example.json"
EXAMPLE_POLICY = REPO / "scripts" / "halofpx-strix-maintenance-policy.example.json"
SPEC = importlib.util.spec_from_file_location("halofpx_strix_maintenance", SOURCE)
assert SPEC is not None and SPEC.loader is not None
maintenance = importlib.util.module_from_spec(SPEC)
sys.modules[SPEC.name] = maintenance
SPEC.loader.exec_module(maintenance)

NOW = dt.datetime(2026, 8, 13, 7, 0, 0, tzinfo=dt.timezone.utc)


def digest(path: Path) -> str:
    return hashlib.sha256(path.read_bytes()).hexdigest()


def identity(role: str, *, fresh: bool = False) -> dict[str, Any]:
    if role == "coordinator":
        pid = 3113343 + (10000 if fresh else 0)
        invocation = "f" * 32 if fresh else "0656332b63a140eab7214627baa43253"
        unit = maintenance.PROTECTED_UNITS[role]
        host = "nimo-1"
        port = 8081
        health = "c" * 64
        tick = 101 + (1000 if fresh else 0)
        mono = 10001 + (100000 if fresh else 0)
    else:
        pid = 2248760 + (10000 if fresh else 0)
        invocation = "e" * 32 if fresh else "d15fe49610274e77bd9a3d84a0b791a5"
        unit = maintenance.PROTECTED_UNITS[role]
        host = "nimo-2"
        port = 50052
        health = None
        tick = 202 + (1000 if fresh else 0)
        mono = 20002 + (100000 if fresh else 0)
    return {
        "role": role,
        "host": host,
        "unit": unit,
        "pid": pid,
        "invocation_id": invocation,
        "nrestarts": 1,
        "process_start_ticks": tick,
        "start_monotonic_us": mono,
        "executable_sha256": "a" * 64,
        "argv_sha256": "b" * 64,
        "control_group": f"/system.slice/{unit}",
        "listener_port": port,
        "listener_pid": pid,
        "health_sha256": health,
    }


def authority() -> dict[str, Any]:
    return {
        "repository": "JCFrags/HaloFPX",
        "issue_number": 41,
        "kind": "github_issue_comment",
        "url": maintenance.ISSUE41_TRACKER + "#issuecomment-1234567890",
        "node_id": "IC_kwDOabcdefgh1234",
        "issuer_login": "JCFrags",
        "issuer_account_id": 12345678,
        "owner_login": "JCFrags",
        "owner_account_id": 12345678,
    }


def authorization_value(evidence_root: Path, commit: str) -> dict[str, Any]:
    plan_path = REPO / "scripts" / "halofpx-strix-ab-plan.example.json"
    adapter_policy_path = REPO / "scripts" / "halofpx-strix-ab-cachyos-policy.example.json"
    return {
        "schema": maintenance.AUTHORIZATION_SCHEMA,
        "authorization_id": "issue41-offline-domain-20260812-a",
        "issue": 41,
        "execution_scope": "offline-domain-simulation",
        "approval_statement": maintenance.OFFLINE_EXAMPLE_STATEMENT,
        "authority": authority(),
        "window": {
            "not_before_utc": "2026-08-13T06:00:00Z",
            "expires_utc": "2026-08-13T08:00:00Z",
        },
        "nonce": "offline-domain-nonce-0123456789abcdef",
        "repository": {
            "url": "https://github.com/JCFrags/HaloFPX.git",
            "commit": commit,
        },
        "incident": {
            "manifest_path": maintenance.ISSUE41_MANIFEST_RELATIVE.as_posix(),
            "manifest_sha256": maintenance.ISSUE41_MANIFEST_SHA256,
        },
        "adapter": {
            "plan_sha256": digest(plan_path),
            "policy_sha256": digest(adapter_policy_path),
            "schedule_index": 0,
        },
        "allowed_disposable": {
            "evidence_root": str(evidence_root.resolve()),
            "unit_prefix": "halofpx-ab-",
            "coordinator_port": 18080,
            "worker_port": 50252,
        },
        "production_before": {
            "coordinator": identity("coordinator"),
            "worker": identity("worker"),
        },
        "recovery_probe": {
            "request_sha256": "d" * 64,
            "prompt_tokens": 5,
            "generated_tokens": 1,
            "world_size": 2,
            "performance_result": False,
        },
    }


def policy_value(authorization_sha256: str, commit: str) -> dict[str, Any]:
    return {
        "schema": maintenance.POLICY_SCHEMA,
        "issue": 41,
        "target_execution_enabled": False,
        "authorization": {
            "receipt_sha256": authorization_sha256,
            "authority": authority(),
        },
        "repository": {
            "url": "https://github.com/JCFrags/HaloFPX.git",
            "commit": commit,
        },
        "incident": {
            "manifest_path": maintenance.ISSUE41_MANIFEST_RELATIVE.as_posix(),
            "manifest_sha256": maintenance.ISSUE41_MANIFEST_SHA256,
        },
        "adapter": {
            "plan_path": "scripts/halofpx-strix-ab-plan.example.json",
            "plan_sha256": digest(REPO / "scripts" / "halofpx-strix-ab-plan.example.json"),
            "policy_path": "scripts/halofpx-strix-ab-cachyos-policy.example.json",
            "policy_sha256": digest(REPO / "scripts" / "halofpx-strix-ab-cachyos-policy.example.json"),
            "schedule_index": 0,
        },
        "timeouts": {
            "stop_seconds": 30,
            "start_seconds": 900,
            "cleanup_seconds": 60,
            "request_seconds": 900,
        },
    }


def write_json(path: Path, value: Any) -> None:
    path.write_text(json.dumps(value, indent=2, sort_keys=True) + "\n", encoding="utf-8")


class FakeRunner:
    offline_fake = True

    def __init__(self, auth: dict[str, Any]):
        self.auth = auth
        self.history: list[str] = []
        self.active = {"coordinator": True, "worker": True}
        self.current = copy.deepcopy(auth["production_before"])
        self.kernel_calls = 0
        self.census_calls = 0
        self.foreign_pid: int | None = None
        self.foreign_on_census_call: int | None = None
        self.incomplete_census = False
        self.fail_adapter = False
        self.bad_adapter_receipt = False
        self.fail_cleanup = False
        self.fail_worker_start = False
        self.fail_coordinator_start = False
        self.stale_recovery_role: str | None = None
        self.bad_probe = False
        self.kernel_delta = False
        self.bad_absence_role: str | None = None

    def snapshot_production(self) -> dict[str, Any]:
        self.history.append("snapshot")
        return {
            "schema": "halofpx.strix-maintenance-production-snapshot.v1",
            "complete": True,
            "roles": copy.deepcopy(self.current),
            "errors": [],
        }

    def kernel_baseline(self) -> dict[str, Any]:
        self.history.append("kernel")
        self.kernel_calls += 1
        delta = 1 if self.kernel_delta and self.kernel_calls > 1 else 0
        return {
            "schema": maintenance.KERNEL_BASELINE_SCHEMA,
            "complete": True,
            "hosts": {
                host: {
                    "boot_id": host + "-boot",
                    "monotonic_ns": self.kernel_calls * 100,
                    "journal_cursor": f"{host}-cursor-{self.kernel_calls}",
                    "global_oom_count": delta,
                    "oom_kill_count": 0,
                    "amdgpu_fault_count": 0,
                    "kfd_fault_count": 0,
                    "gpu_reset_count": 0,
                    "errors": [],
                }
                for host in ("nimo-1", "nimo-2")
            },
            "errors": [],
        }

    def gpu_census(self) -> dict[str, Any]:
        self.history.append("census")
        self.census_calls += 1
        hosts: dict[str, Any] = {}
        for host, role in (("nimo-1", "coordinator"), ("nimo-2", "worker")):
            owners = []
            if self.active[role]:
                current = self.current[role]
                owners.append({
                    "pid": current["pid"], "unit": current["unit"],
                    "control_group": current["control_group"], "gpu_active_kib": 1024,
                })
            foreign_now = self.foreign_pid is not None and (
                self.foreign_on_census_call is None or
                self.census_calls == self.foreign_on_census_call)
            if foreign_now and host == "nimo-2":
                owners.append({
                    "pid": self.foreign_pid, "unit": "foreign.service",
                    "control_group": "/user.slice/foreign.service", "gpu_active_kib": 1,
                })
            hosts[host] = {
                "devices": ["/dev/kfd", "/dev/dri/renderD128"],
                "owners": owners,
                "errors": [],
            }
        return {
            "schema": maintenance.GPU_CENSUS_SCHEMA,
            "elevated": True,
            "complete": not self.incomplete_census,
            "hosts": hosts,
            "errors": ["incomplete"] if self.incomplete_census else [],
        }

    def stop_production(self, stopped: maintenance.ProductionIdentity, timeout_seconds: int) -> dict[str, Any]:
        self.history.append("stop-" + stopped.role)
        self.active[stopped.role] = False
        return {
            "host": stopped.host,
            "unit": stopped.unit,
            "stopped_identity_sha256": stopped.digest,
            "active": False,
            "main_pid": 0,
            "listener_pids": [],
            "control_group_absent": True,
        }

    def prove_production_absent(self, role: str) -> dict[str, Any]:
        self.history.append("absent-" + role)
        if self.bad_absence_role == role:
            return {"role": role, "unit": maintenance.PROTECTED_UNITS[role], "active": True,
                    "main_pid": 99, "listener_pids": [99]}
        return {"role": role, "unit": maintenance.PROTECTED_UNITS[role], "active": False,
                "main_pid": 0, "listener_pids": []}

    def run_adapter(self, plan_path: Path, policy_path: Path, evidence_root: Path) -> bytes:
        self.history.append("adapter")
        if self.fail_adapter:
            raise maintenance.MaintenanceError("synthetic adapter failure")
        plan = maintenance.core.load_plan(plan_path)
        schedule = maintenance.core.make_schedule(plan)
        value = {
            "schema": maintenance.adapter.RECEIPT_SCHEMA,
            "issue": 37,
            "experiment_id": plan["experiment_id"],
            "plan_sha256": maintenance.core.plan_digest(plan),
            "policy_sha256": digest(policy_path),
            "policy_binding": {"path": "policy.raw", "size_bytes": 1, "sha256": digest(policy_path)},
            "schedule_index": 0,
            "entry": schedule["entries"][0],
            "input_bindings": {},
            "model_binding": {},
            "preflight_sha256": {},
            "production_before": {"inactive": True},
            "production_after": {"inactive": True},
            "gpu_admission_before_intent": {},
            "model_binding_after": {},
            "cycles": [],
            "errors": [],
            "outcome": {"status": "success", "failure_code": None},
            "execution_qualified": False,
            "measurement_ready": False,
            "performance_claim": False,
        }
        if self.bad_adapter_receipt:
            value["performance_claim"] = True
        return json.dumps(value, sort_keys=True).encode("utf-8") + b"\n"

    def cleanup_adapter(self, timeout_seconds: int) -> dict[str, Any]:
        self.history.append("cleanup-adapter")
        if self.fail_cleanup:
            raise maintenance.MaintenanceError("synthetic cleanup failure")
        return {
            "complete": True,
            "stop_order": ["coordinator", "worker"],
            "cleanup_order": ["coordinator", "worker"],
            "units_absent": True,
            "ports_closed": True,
            "paths_removed": True,
            "errors": [],
        }

    def start_production(self, role: str, timeout_seconds: int) -> dict[str, Any]:
        self.history.append("start-" + role)
        if role == "worker" and self.fail_worker_start:
            raise maintenance.MaintenanceError("synthetic worker start failure")
        if role == "coordinator" and self.fail_coordinator_start:
            raise maintenance.MaintenanceError("synthetic coordinator start failure")
        value = identity(role, fresh=self.stale_recovery_role != role)
        self.current[role] = copy.deepcopy(value)
        self.active[role] = True
        return value

    def prove_recovery_ready(self, recovered: maintenance.ProductionIdentity, timeout_seconds: int) -> dict[str, Any]:
        self.history.append("ready-" + recovered.role)
        return {
            "role": recovered.role,
            "identity_sha256": recovered.digest,
            "listener_pids": [recovered.pid],
            "ready": True,
            "health": ({"status": 200, "body_sha256": recovered.health_sha256}
                       if recovered.role == "coordinator" else None),
            "rpc_protocol": "4.0.1" if recovered.role == "worker" else None,
        }

    def minimal_two_rank_inference(
        self, coordinator: maintenance.ProductionIdentity, worker: maintenance.ProductionIdentity,
        request_sha256: str, timeout_seconds: int,
    ) -> dict[str, Any]:
        self.history.append("probe")
        value = {
            "schema": maintenance.PROBE_SCHEMA,
            "request_sha256": request_sha256,
            "prompt_tokens": 5,
            "generated_tokens": 1,
            "world_size": 2,
            "coordinator_identity_sha256": coordinator.digest,
            "worker_identity_sha256": worker.digest,
            "completed": True,
            "performance_result": False,
        }
        if self.bad_probe:
            value["world_size"] = 1
        return value


class MaintenanceControllerTest(unittest.TestCase):
    def setUp(self) -> None:
        self.temp = tempfile.TemporaryDirectory()
        self.root = Path(self.temp.name)
        self.commit = subprocess.check_output(
            ["git", "-C", str(REPO), "rev-parse", "HEAD"], text=True).strip()
        self.evidence = self.root / "evidence"
        self.authorization = authorization_value(self.evidence, self.commit)
        self.auth_path = self.root / "authorization.json"
        write_json(self.auth_path, self.authorization)
        self.policy = policy_value(digest(self.auth_path), self.commit)
        self.policy_path = self.root / "policy.json"
        write_json(self.policy_path, self.policy)
        self.runner = FakeRunner(self.authorization)

    def tearDown(self) -> None:
        self.temp.cleanup()

    def execute(self) -> Path:
        return maintenance.execute_offline_domain(
            REPO, self.policy_path, self.auth_path, self.runner, now=NOW)

    def failure(self) -> maintenance.MaintenanceRunFailed:
        with self.assertRaises(maintenance.MaintenanceRunFailed) as raised:
            self.execute()
        return raised.exception

    def test_happy_path_orders_shutdown_adapter_cleanup_and_recovery(self) -> None:
        terminal_path = self.execute()
        terminal = json.loads(terminal_path.read_text(encoding="utf-8"))
        self.assertEqual(terminal["status"], "success")
        self.assertTrue(terminal["recovery_complete"])
        self.assertIsNone(terminal["performance_result"])
        self.assertEqual(self.runner.history, [
            "snapshot", "kernel", "census",
            "stop-coordinator", "absent-coordinator", "census",
            "stop-worker", "absent-worker", "census",
            "adapter", "cleanup-adapter", "census",
            "start-worker", "ready-worker", "start-coordinator", "ready-coordinator",
            "census", "probe", "kernel",
        ])
        self.assertTrue((self.evidence / "adapter-receipt.raw.json").is_file())
        self.assertTrue((self.evidence / "SHA256SUMS").is_file())
        event_names = [path.name for path in sorted((self.evidence / "events").glob("*.json"))]
        self.assertTrue(any("post-stop-empty-gpu-census" in name for name in event_names))
        self.assertTrue(any("minimal-two-rank-inference-contract" in name for name in event_names))

    def test_pre_stop_foreign_owner_refuses_before_mutation(self) -> None:
        self.runner.foreign_pid = 999
        failure = self.failure()
        self.assertNotIn("stop-coordinator", self.runner.history)
        terminal = json.loads(failure.terminal_path.read_text(encoding="utf-8"))
        self.assertFalse(terminal["first_mutation"])

    def test_incomplete_elevated_census_is_not_empty(self) -> None:
        self.runner.incomplete_census = True
        self.failure()
        self.assertNotIn("stop-coordinator", self.runner.history)

    def test_between_stop_foreign_owner_refuses_worker_stop_and_adapter(self) -> None:
        self.runner.foreign_pid = 999
        self.runner.foreign_on_census_call = 2
        self.failure()
        self.assertIn("stop-coordinator", self.runner.history)
        self.assertNotIn("stop-worker", self.runner.history)
        self.assertNotIn("adapter", self.runner.history)

    def test_post_stop_foreign_owner_refuses_adapter_and_recovers(self) -> None:
        self.runner.foreign_pid = 999
        self.runner.foreign_on_census_call = 3
        failure = self.failure()
        terminal = json.loads(failure.terminal_path.read_text(encoding="utf-8"))
        self.assertNotIn("adapter", self.runner.history)
        self.assertTrue(terminal["recovery_complete"])

    def test_production_identity_drift_refuses_before_mutation(self) -> None:
        self.runner.current["worker"]["pid"] += 1
        self.runner.current["worker"]["listener_pid"] += 1
        self.failure()
        self.assertNotIn("stop-coordinator", self.runner.history)

    def test_coordinator_absence_failure_forbids_worker_stop_but_recovers(self) -> None:
        self.runner.bad_absence_role = "coordinator"
        failure = self.failure()
        self.assertNotIn("stop-worker", self.runner.history)
        self.assertNotIn("start-worker", self.runner.history)
        self.assertIn("ready-worker", self.runner.history)
        self.assertIn("start-coordinator", self.runner.history)
        terminal = json.loads(failure.terminal_path.read_text(encoding="utf-8"))
        self.assertEqual(
            terminal["production_recovered"]["worker"]["pid"],
            self.authorization["production_before"]["worker"]["pid"])

    def test_adapter_failure_still_cleans_then_recovers(self) -> None:
        self.runner.fail_adapter = True
        self.failure()
        self.assertLess(self.runner.history.index("cleanup-adapter"), self.runner.history.index("start-worker"))
        self.assertLess(self.runner.history.index("start-worker"), self.runner.history.index("start-coordinator"))

    def test_adapter_receipt_cannot_claim_performance(self) -> None:
        self.runner.bad_adapter_receipt = True
        failure = self.failure()
        terminal = json.loads(failure.terminal_path.read_text(encoding="utf-8"))
        self.assertTrue(any(item["stage"] == "maintenance body" for item in terminal["errors"]))
        self.assertIn("cleanup-adapter", self.runner.history)
        self.assertIn("probe", self.runner.history)

    def test_cleanup_failure_is_retained_and_blocks_production_restart(self) -> None:
        self.runner.fail_cleanup = True
        failure = self.failure()
        terminal = json.loads(failure.terminal_path.read_text(encoding="utf-8"))
        self.assertFalse(terminal["recovery_complete"])
        self.assertTrue(any(item["stage"] == "adapter cleanup" for item in terminal["errors"]))
        self.assertNotIn("start-worker", self.runner.history)
        self.assertNotIn("start-coordinator", self.runner.history)
        self.assertNotIn("probe", self.runner.history)

    def test_custody_event_failure_does_not_block_worker_first_recovery(self) -> None:
        original_event = maintenance.EvidenceCustody.event
        injected = False

        def fail_cleanup_event(
            custody: maintenance.EvidenceCustody, stage: str, status: str,
            observation: Any,
        ) -> Path:
            nonlocal injected
            if stage == "adapter cleanup" and not injected:
                injected = True
                raise OSError("synthetic custody write failure")
            return original_event(custody, stage, status, observation)

        with mock.patch.object(maintenance.EvidenceCustody, "event", new=fail_cleanup_event):
            failure = self.failure()
        terminal = json.loads(failure.terminal_path.read_text(encoding="utf-8"))
        self.assertTrue(injected)
        self.assertTrue(terminal["recovery_complete"])
        self.assertTrue(any(
            item["stage"] == "evidence custody" and
            "after adapter cleanup" in item["detail"]
            for item in terminal["errors"]))
        self.assertLess(
            self.runner.history.index("cleanup-adapter"),
            self.runner.history.index("start-worker"))
        self.assertLess(
            self.runner.history.index("start-worker"),
            self.runner.history.index("start-coordinator"))

    def test_worker_recovery_failure_forbids_coordinator_start(self) -> None:
        self.runner.fail_worker_start = True
        failure = self.failure()
        self.assertNotIn("start-coordinator", self.runner.history)
        terminal = json.loads(failure.terminal_path.read_text(encoding="utf-8"))
        self.assertFalse(terminal["recovery_complete"])

    def test_stale_worker_identity_forbids_coordinator_start(self) -> None:
        self.runner.stale_recovery_role = "worker"
        self.failure()
        self.assertNotIn("start-coordinator", self.runner.history)

    def test_coordinator_recovery_failure_is_terminal(self) -> None:
        self.runner.fail_coordinator_start = True
        failure = self.failure()
        terminal = json.loads(failure.terminal_path.read_text(encoding="utf-8"))
        self.assertFalse(terminal["recovery_complete"])
        self.assertNotIn("probe", self.runner.history)

    def test_health_is_not_a_substitute_for_two_rank_inference_contract(self) -> None:
        self.runner.bad_probe = True
        failure = self.failure()
        terminal = json.loads(failure.terminal_path.read_text(encoding="utf-8"))
        self.assertTrue(terminal["services_ready"])
        self.assertTrue(terminal["recovery_census_complete"])
        self.assertFalse(terminal["recovery_probe_complete"])
        self.assertFalse(terminal["recovery_complete"])
        self.assertTrue(any(item["stage"] == "minimal two-rank inference contract" for item in terminal["errors"]))

    def test_recovered_census_failure_blocks_inference_contract(self) -> None:
        self.runner.foreign_pid = 999
        self.runner.foreign_on_census_call = 5
        failure = self.failure()
        terminal = json.loads(failure.terminal_path.read_text(encoding="utf-8"))
        self.assertTrue(terminal["services_ready"])
        self.assertFalse(terminal["recovery_census_complete"])
        self.assertFalse(terminal["recovery_probe_complete"])
        self.assertFalse(terminal["recovery_complete"])
        self.assertTrue(any(
            item["stage"] == "recovered production GPU census"
            for item in terminal["errors"]))
        self.assertNotIn("probe", self.runner.history)

    def test_kernel_oom_or_fault_delta_fails_after_recovery(self) -> None:
        self.runner.kernel_delta = True
        failure = self.failure()
        terminal = json.loads(failure.terminal_path.read_text(encoding="utf-8"))
        self.assertTrue(any(item["stage"] == "kernel after" for item in terminal["errors"]))

    def test_receipt_replay_refuses_same_frozen_evidence_root(self) -> None:
        self.execute()
        second = FakeRunner(self.authorization)
        with self.assertRaisesRegex(maintenance.MaintenanceError, "must not already exist"):
            maintenance.execute_offline_domain(
                REPO, self.policy_path, self.auth_path, second, now=NOW)
        self.assertEqual(second.history, [])

    def test_non_fake_runner_is_refused_before_input_or_mutation(self) -> None:
        class NotFake:
            offline_fake = False
        with self.assertRaisesRegex(maintenance.MaintenanceError, "offline fake"):
            maintenance.execute_offline_domain(
                REPO, self.policy_path, self.auth_path, NotFake(), now=NOW)

    def test_authorization_duplicate_extra_digest_and_expiry_refuse(self) -> None:
        raw = self.auth_path.read_text(encoding="utf-8")
        duplicate = raw.replace('"issue": 41,', '"issue": 41,\n  "issue": 41,', 1).encode()
        with self.assertRaisesRegex(maintenance.MaintenanceError, "duplicate JSON key"):
            maintenance.load_authorization_bytes(
                duplicate, expected_sha256=hashlib.sha256(duplicate).hexdigest(), now=NOW)
        extra = copy.deepcopy(self.authorization)
        extra["unexpected"] = True
        extra_bytes = json.dumps(extra).encode()
        with self.assertRaisesRegex(maintenance.MaintenanceError, "wrong closed field set"):
            maintenance.load_authorization_bytes(
                extra_bytes, expected_sha256=hashlib.sha256(extra_bytes).hexdigest(), now=NOW)
        with self.assertRaisesRegex(maintenance.MaintenanceError, "tracked policy digest"):
            maintenance.load_authorization_bytes(self.auth_path.read_bytes(), expected_sha256="0" * 64, now=NOW)
        with self.assertRaisesRegex(maintenance.MaintenanceError, "not active"):
            maintenance.load_authorization_bytes(
                self.auth_path.read_bytes(), expected_sha256=digest(self.auth_path),
                now=dt.datetime(2026, 8, 13, 8, 0, tzinfo=dt.timezone.utc))

    def test_authority_must_be_exact_tracked_issue_comment_and_owner(self) -> None:
        changed = copy.deepcopy(self.authorization)
        changed["authority"]["issuer_login"] = "attacker"
        raw = json.dumps(changed).encode()
        with self.assertRaisesRegex(maintenance.MaintenanceError, "issuer/owner"):
            maintenance.load_authorization_bytes(
                raw, expected_sha256=hashlib.sha256(raw).hexdigest(), now=NOW)

    def test_policy_and_authorization_authority_mismatch_refuses(self) -> None:
        changed = copy.deepcopy(self.policy)
        changed["authorization"]["authority"]["node_id"] = "IC_kwDOdifferent1234"
        write_json(self.policy_path, changed)
        with self.assertRaisesRegex(maintenance.MaintenanceError, "authority differs"):
            maintenance.validate_inputs(REPO, self.policy_path, self.auth_path, now=NOW)

    def test_changed_pr51_input_bytes_refuse(self) -> None:
        changed = copy.deepcopy(self.policy)
        changed["adapter"]["plan_sha256"] = "0" * 64
        write_json(self.policy_path, changed)
        with self.assertRaisesRegex(maintenance.MaintenanceError, "raw bytes differ"):
            maintenance.validate_inputs(REPO, self.policy_path, self.auth_path, now=NOW)

    def test_dual_strix_scope_cannot_cross_offline_domain_seam(self) -> None:
        changed = copy.deepcopy(self.authorization)
        changed["execution_scope"] = "dual-strix-maintenance"
        changed["approval_statement"] = maintenance.APPROVAL_STATEMENT
        write_json(self.auth_path, changed)
        self.policy = policy_value(digest(self.auth_path), self.commit)
        write_json(self.policy_path, self.policy)
        with self.assertRaisesRegex(maintenance.MaintenanceError, "offline-domain-simulation"):
            self.execute()

    def test_cli_validate_refuses_dual_strix_scope(self) -> None:
        changed = copy.deepcopy(self.authorization)
        changed["execution_scope"] = "dual-strix-maintenance"
        changed["approval_statement"] = maintenance.APPROVAL_STATEMENT
        write_json(self.auth_path, changed)
        write_json(self.policy_path, policy_value(digest(self.auth_path), self.commit))
        result = subprocess.run([
            sys.executable, str(SOURCE), "--repository-root", str(REPO),
            "--policy", str(self.policy_path), "--authorization", str(self.auth_path),
            "--now-utc", "2026-08-13T07:00:00Z", "validate",
        ], capture_output=True, text=True, check=False)
        self.assertEqual(result.returncode, 1)
        self.assertIn("offline-domain-simulation only", result.stderr)

    def test_cli_execute_is_hard_disabled_and_never_mentions_ssh(self) -> None:
        result = subprocess.run([
            sys.executable, str(SOURCE), "--repository-root", str(REPO),
            "--policy", str(self.root / "missing-policy.json"),
            "--authorization", str(self.root / "missing-authorization.json"),
            "--now-utc", "2026-08-13T07:00:00Z", "execute",
        ], capture_output=True, text=True, check=False)
        self.assertEqual(result.returncode, 1)
        self.assertIn("hard-disabled", result.stderr)
        self.assertNotIn("ssh", " ".join(self.runner.history).lower())

    def test_hard_disabled_feature_never_constructs_pr51_ssh_runner(self) -> None:
        with mock.patch.object(
            maintenance.adapter, "SshCachyRunner",
            side_effect=AssertionError("SSH Runner construction is forbidden"),
        ) as constructor, mock.patch("sys.stderr", new_callable=io.StringIO):
            result = maintenance.main([
                "--repository-root", str(REPO),
                "--policy", str(self.root / "missing-policy.json"),
                "--authorization", str(self.root / "missing-authorization.json"),
                "--now-utc", "2026-08-13T07:00:00Z",
                "execute",
            ])
        self.assertEqual(result, 1)
        constructor.assert_not_called()

    def test_source_contract_has_no_real_runner_and_literal_off_gate(self) -> None:
        source = SOURCE.read_text(encoding="utf-8")
        self.assertIn("TARGET_EXECUTION_ENABLED = False", source)
        self.assertNotIn("class Ssh", source)
        self.assertNotIn("ssh", source.lower())
        self.assertNotIn("import subprocess", source)
        self.assertNotIn("import socket", source)
        self.assertNotIn("import requests", source)
        self.assertNotIn("import urllib", source)
        self.assertNotIn("subprocess.Popen", source)
        self.assertIn("only an explicit offline fake Runner is admitted", source)

    def test_census_identity_authority_is_scoped_by_host_and_pid(self) -> None:
        coordinator_raw = identity("coordinator")
        worker_raw = identity("worker")
        worker_raw["pid"] = coordinator_raw["pid"]
        worker_raw["listener_pid"] = coordinator_raw["pid"]
        coordinator = maintenance.parse_identity(coordinator_raw, "coordinator", "coordinator")
        worker = maintenance.parse_identity(worker_raw, "worker", "worker")
        runner = FakeRunner({
            "production_before": {"coordinator": coordinator_raw, "worker": worker_raw}
        })
        observed = maintenance.validate_census(
            runner.gpu_census(),
            {"nimo-1": {coordinator.pid}, "nimo-2": {worker.pid}},
            "same numeric PID on different hosts",
            identities=(coordinator, worker))
        self.assertTrue(observed["complete"])

    def test_tracked_example_pair_is_exact_offline_only_and_digest_bound(self) -> None:
        auth_bytes = EXAMPLE_AUTHORIZATION.read_bytes()
        policy = maintenance.load_policy_bytes(EXAMPLE_POLICY.read_bytes())
        self.assertEqual(policy.authorization_sha256, hashlib.sha256(auth_bytes).hexdigest())
        self.assertFalse(json.loads(EXAMPLE_POLICY.read_text(encoding="utf-8"))[
            "target_execution_enabled"])
        raw = json.loads(auth_bytes)
        self.assertEqual(raw["execution_scope"], "offline-domain-simulation")
        self.assertEqual(raw["approval_statement"], maintenance.OFFLINE_EXAMPLE_STATEMENT)
        self.assertEqual(raw["authority"]["issuer_account_id"], 222912166)
        if os.name == "nt":
            _, _, _, authorization, _ = maintenance.validate_inputs(
                REPO, EXAMPLE_POLICY, EXAMPLE_AUTHORIZATION, now=NOW)
            self.assertEqual(authorization.authorization_id, "issue41-offline-domain-example-v1")


if __name__ == "__main__":
    unittest.main()
