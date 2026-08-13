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


def rebuild_unsigned_bundle_bindings(root: Path) -> None:
    """Rebuild local hashes after adversarial edits; this is not a signature."""
    rows = []
    for path in sorted(root.rglob("*")):
        if path.is_file() and path.name not in {"SHA256SUMS", "COMMITTED.json"}:
            rows.append(f"{digest(path)}  {path.relative_to(root).as_posix()}\n")
    hashes = root / "SHA256SUMS"
    hashes.write_text("".join(rows), encoding="ascii", newline="")
    marker = json.loads((root / "COMMITTED.json").read_text(encoding="utf-8"))
    marker["terminal_sha256"] = digest(root / "terminal.json")
    marker["hashes_sha256"] = digest(hashes)
    (root / "COMMITTED.json").write_bytes(maintenance._pretty_json_bytes(marker))


class FakeRunner:
    offline_fake = True

    def __init__(self, auth: dict[str, Any]):
        self.auth = auth
        self.history: list[str] = []
        self.active = {"coordinator": True, "worker": True}
        self.current = copy.deepcopy(auth["production_before"])
        self.snapshot_calls = 0
        self.kernel_calls = 0
        self.census_calls = 0
        self.foreign_pid: int | None = None
        self.foreign_on_census_call: int | None = None
        self.incomplete_census = False
        self.fail_adapter = False
        self.bad_adapter_receipt = False
        self.fail_cleanup = False
        self.fail_cleanup_after_effect = False
        self.adapter_present = False
        self.fail_worker_start = False
        self.fail_coordinator_start = False
        self.fail_worker_start_after_effect = False
        self.fail_coordinator_start_after_effect = False
        self.fail_stop_after_effect_role: str | None = None
        self.stale_recovery_role: str | None = None
        self.backward_recovery_role: str | None = None
        self.drift_recovery_role: str | None = None
        self.bad_probe = False
        self.kernel_delta = False
        self.bad_absence_role: str | None = None
        self.residual_control_group_role: str | None = None
        self.drift_role_on_snapshot_call: tuple[str, int] | None = None
        self.reappear_stopped_before_recovery = False
        self.reappear_on_snapshot_call = 4
        self.drop_role_on_snapshot_call: tuple[str, int] | None = None

    def snapshot_production(self) -> dict[str, Any]:
        self.history.append("snapshot")
        self.snapshot_calls += 1
        if self.drop_role_on_snapshot_call is not None:
            role, call = self.drop_role_on_snapshot_call
            if self.snapshot_calls == call:
                self.active[role] = False
        roles: dict[str, Any] = {}
        for role in ("coordinator", "worker"):
            active = self.active[role]
            if self.bad_absence_role == role and not active:
                active = True
            if self.reappear_stopped_before_recovery and \
                    self.snapshot_calls >= self.reappear_on_snapshot_call and not active:
                active = True
            identity_value = copy.deepcopy(self.current[role]) if active else None
            if self.drift_role_on_snapshot_call == (role, self.snapshot_calls) and \
                    identity_value is not None:
                identity_value["pid"] += 777
                identity_value["listener_pid"] += 777
            roles[role] = {
                "active": active,
                "host": identity_value["host"] if identity_value is not None else self.current[role]["host"],
                "unit": identity_value["unit"] if identity_value is not None else self.current[role]["unit"],
                "unit_active_state": "active" if active else "inactive",
                "unit_sub_state": "running" if active else "dead",
                "main_pid": identity_value["pid"] if identity_value is not None else 0,
                "listener_pids": [identity_value["pid"]] if identity_value is not None else [],
                "control_group": (
                    identity_value["control_group"] if identity_value is not None
                    else self.current[role]["control_group"]),
                "control_group_exists": active or self.residual_control_group_role == role,
                "control_group_pids": (
                    [identity_value["pid"]] if identity_value is not None else
                    ([991337] if self.residual_control_group_role == role else [])),
                "identity": identity_value,
            }
        return {
            "schema": maintenance.PRODUCTION_SNAPSHOT_SCHEMA,
            "complete": True,
            "roles": roles,
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
            if self.active[role] or (
                    self.reappear_stopped_before_recovery and self.census_calls >= 4):
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
        if self.fail_stop_after_effect_role == stopped.role:
            raise maintenance.MaintenanceError(
                f"synthetic {stopped.role} response lost after stop")
        return {
            "host": stopped.host,
            "unit": stopped.unit,
            "stopped_identity_sha256": stopped.digest,
            "active": False,
            "main_pid": 0,
            "listener_pids": [],
            "control_group_absent": True,
        }

    def run_adapter(self, plan_path: Path, policy_path: Path, evidence_root: Path) -> bytes:
        self.history.append("adapter")
        self.adapter_present = True
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
        self.adapter_present = False
        if self.fail_cleanup_after_effect:
            raise maintenance.MaintenanceError("synthetic cleanup response lost after effect")
        return {
            "complete": True,
            "stop_order": ["coordinator", "worker"],
            "cleanup_order": ["coordinator", "worker"],
            "units_absent": True,
            "ports_closed": True,
            "paths_removed": True,
            "errors": [],
        }

    def prove_adapter_absent(self) -> dict[str, Any]:
        self.history.append("absent-adapter")
        absent = not self.adapter_present
        return {
            "units_absent": absent,
            "ports_closed": absent,
            "paths_removed": absent,
            "errors": [] if absent else ["synthetic adapter residue"],
        }

    def start_production(self, role: str, timeout_seconds: int) -> dict[str, Any]:
        self.history.append("start-" + role)
        if role == "worker" and self.fail_worker_start:
            raise maintenance.MaintenanceError("synthetic worker start failure")
        if role == "coordinator" and self.fail_coordinator_start:
            raise maintenance.MaintenanceError("synthetic coordinator start failure")
        value = identity(role, fresh=self.stale_recovery_role != role)
        if self.backward_recovery_role == role:
            value["process_start_ticks"] = self.auth["production_before"][role]["process_start_ticks"] - 1
            value["start_monotonic_us"] = self.auth["production_before"][role]["start_monotonic_us"] - 1
        if self.drift_recovery_role == role:
            value["control_group"] = f"/drift.slice/{value['unit']}"
            if role == "coordinator":
                value["health_sha256"] = "9" * 64
        self.current[role] = copy.deepcopy(value)
        self.active[role] = True
        if role == "worker" and self.fail_worker_start_after_effect:
            raise maintenance.MaintenanceError("synthetic worker response lost after start")
        if role == "coordinator" and self.fail_coordinator_start_after_effect:
            raise maintenance.MaintenanceError("synthetic coordinator response lost after start")
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
        committed_path = self.execute()
        self.assertEqual(committed_path, self.evidence / "COMMITTED.json")
        terminal = maintenance.verify_committed_bundle(self.evidence)
        self.assertEqual(terminal["status"], "success")
        self.assertTrue(terminal["recovery_complete"])
        self.assertIsNone(terminal["performance_result"])
        self.assertEqual(self.runner.history, [
            "snapshot", "kernel", "census",
            "stop-coordinator", "snapshot", "census",
            "stop-worker", "snapshot", "census",
            "adapter", "cleanup-adapter", "absent-adapter", "snapshot", "census",
            "start-worker", "snapshot", "ready-worker",
            "start-coordinator", "snapshot", "ready-coordinator",
            "census", "probe", "kernel",
            "snapshot",
        ])
        self.assertTrue((self.evidence / "adapter-receipt.raw.json").is_file())
        self.assertTrue((self.evidence / "SHA256SUMS").is_file())
        committed = json.loads((self.evidence / "COMMITTED.json").read_text(encoding="utf-8"))
        self.assertEqual(committed["schema"], maintenance.COMMIT_SCHEMA)
        self.assertEqual(committed["terminal_sha256"], digest(self.evidence / "terminal.json"))
        self.assertEqual(committed["hashes_sha256"], digest(self.evidence / "SHA256SUMS"))
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

    def test_stopped_old_identities_cannot_reappear_as_preserved_recovery(self) -> None:
        self.runner.reappear_stopped_before_recovery = True
        with self.assertRaises(maintenance.MaintenanceRunFailed) as caught:
            self.execute()
        terminal = json.loads(caught.exception.terminal_path.read_text(encoding="utf-8"))
        self.assertFalse(terminal["services_ready"])
        self.assertFalse(terminal["recovery_complete"])
        self.assertEqual(terminal["production_recovered"], {})
        self.assertNotIn("start-worker", self.runner.history)
        self.assertNotIn("start-coordinator", self.runner.history)
        self.assertNotIn("probe", self.runner.history)
        self.assertTrue(any(
            item["stage"] == "pre-recovery reconciled GPU census"
            for item in terminal["errors"]))

    def test_production_identity_drift_refuses_before_mutation(self) -> None:
        self.runner.current["worker"]["pid"] += 1
        self.runner.current["worker"]["listener_pid"] += 1
        self.failure()
        self.assertNotIn("stop-coordinator", self.runner.history)

    def test_coordinator_stop_postcondition_mismatch_forbids_worker_stop(self) -> None:
        self.runner.bad_absence_role = "coordinator"
        failure = self.failure()
        self.assertNotIn("stop-worker", self.runner.history)
        self.assertNotIn("start-worker", self.runner.history)
        self.assertNotIn("ready-worker", self.runner.history)
        self.assertNotIn("start-coordinator", self.runner.history)
        terminal = json.loads(failure.terminal_path.read_text(encoding="utf-8"))
        self.assertFalse(terminal["recovery_complete"])

    def test_adapter_failure_still_cleans_then_recovers(self) -> None:
        self.runner.fail_adapter = True
        self.failure()
        self.assertLess(self.runner.history.index("cleanup-adapter"), self.runner.history.index("start-worker"))
        self.assertLess(self.runner.history.index("start-worker"), self.runner.history.index("start-coordinator"))

    def test_cleanup_lost_response_uses_independent_absence_and_recovers(self) -> None:
        self.runner.fail_cleanup_after_effect = True
        failure = self.failure()
        terminal = json.loads(failure.terminal_path.read_text(encoding="utf-8"))
        self.assertTrue(terminal["recovery_complete"])
        self.assertIn("absent-adapter", self.runner.history)
        self.assertTrue(any(
            item["stage"] == "adapter cleanup actuation" for item in terminal["errors"]))

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
        self.assertTrue(any(
            item["stage"] == "adapter cleanup actuation" for item in terminal["errors"]))
        self.assertTrue(any(item["stage"] == "adapter absence" for item in terminal["errors"]))
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

    def test_mandatory_forward_custody_failure_aborts_body_and_still_recovers(self) -> None:
        original_event = maintenance.EvidenceCustody.event

        for failed_stage, required_history, forbidden_history in (
                ("production before", ["snapshot"], ["kernel", "stop-coordinator", "adapter"]),
                ("kernel before", ["snapshot", "kernel"], ["census", "stop-coordinator", "adapter"]),
                ("pre-stop GPU census", ["census"], ["stop-coordinator", "adapter"]),
                ("coordinator stop", ["stop-coordinator"], ["stop-worker", "adapter"]),
                ("coordinator stop postcondition raw", ["stop-coordinator", "snapshot"],
                 ["stop-worker", "adapter"]),
                ("between-stop GPU census", ["stop-coordinator", "census"], ["stop-worker", "adapter"]),
                ("worker stop", ["stop-worker"], ["adapter"]),
                ("worker stop postcondition raw", ["stop-worker", "snapshot"], ["adapter"]),
                ("post-stop empty GPU census", ["stop-worker", "census"], ["adapter"]),
                ("adapter handoff", ["adapter"], []),
        ):
            with self.subTest(stage=failed_stage):
                self.tearDown()
                self.setUp()
                injected = False

                def fail_forward_event(
                    custody: maintenance.EvidenceCustody, stage: str, status: str,
                    observation: Any,
                ) -> Path:
                    nonlocal injected
                    if stage == failed_stage and not injected:
                        injected = True
                        raise OSError("synthetic mandatory custody failure")
                    return original_event(custody, stage, status, observation)

                with mock.patch.object(
                        maintenance.EvidenceCustody, "event", new=fail_forward_event):
                    failure = self.failure()
                terminal = json.loads(failure.terminal_path.read_text(encoding="utf-8"))
                self.assertTrue(injected)
                self.assertEqual(terminal["status"], "failure")
                self.assertTrue(any(
                    item["stage"] == "evidence custody" and
                    f"after {failed_stage}" in item["detail"]
                    for item in terminal["errors"]))
                self.assertTrue(any(
                    item["stage"] == "maintenance body" and
                    "mandatory evidence custody failed" in item["detail"]
                    for item in terminal["errors"]))
                for item in required_history:
                    self.assertIn(item, self.runner.history)
                for item in forbidden_history:
                    self.assertNotIn(item, self.runner.history)
                if terminal["first_mutation"]:
                    self.assertIn("ready-worker", self.runner.history)
                    self.assertIn("ready-coordinator", self.runner.history)

    def test_lost_stop_postcondition_custody_cannot_reclassify_stale_identity(self) -> None:
        original_event = maintenance.EvidenceCustody.event
        self.runner.reappear_stopped_before_recovery = True
        self.runner.reappear_on_snapshot_call = 3

        def fail_stop_postcondition(
            custody: maintenance.EvidenceCustody, stage: str, status: str,
            observation: Any,
        ) -> Path:
            if stage == "coordinator stop postcondition raw":
                raise OSError("synthetic stop-postcondition custody failure")
            return original_event(custody, stage, status, observation)

        with mock.patch.object(
                maintenance.EvidenceCustody, "event", new=fail_stop_postcondition):
            terminal = json.loads(self.failure().terminal_path.read_text(encoding="utf-8"))
        self.assertFalse(terminal["recovery_complete"])
        self.assertNotIn("start-coordinator", self.runner.history)
        self.assertTrue(any(
            item["stage"] == "pre-recovery reconciled GPU census"
            for item in terminal["errors"]))

    def test_other_role_drift_cannot_discard_observed_stop_absence(self) -> None:
        self.runner.drift_role_on_snapshot_call = ("worker", 2)
        self.runner.reappear_stopped_before_recovery = True
        self.runner.reappear_on_snapshot_call = 3
        terminal = json.loads(self.failure().terminal_path.read_text(encoding="utf-8"))
        self.assertFalse(terminal["recovery_complete"])
        self.assertNotIn("start-coordinator", self.runner.history)
        self.assertEqual(terminal["production_recovered"], {})

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

    def test_backward_recovery_timestamps_forbid_coordinator_start(self) -> None:
        self.runner.backward_recovery_role = "worker"
        self.failure()
        self.assertNotIn("start-coordinator", self.runner.history)

    def test_recovery_cgroup_and_health_authority_cannot_drift(self) -> None:
        for role in ("worker", "coordinator"):
            with self.subTest(role=role):
                self.tearDown()
                self.setUp()
                self.runner.drift_recovery_role = role
                terminal = json.loads(self.failure().terminal_path.read_text(encoding="utf-8"))
                self.assertFalse(terminal["recovery_complete"])
                self.assertTrue(any(
                    item["stage"] == f"{role} recovery" for item in terminal["errors"]))
                if role == "worker":
                    self.assertNotIn("start-coordinator", self.runner.history)

    def test_exact_validators_reject_boolean_integer_aliases(self) -> None:
        stopped = self.runner.stop_production(
            maintenance.parse_identity(
                self.authorization["production_before"]["coordinator"],
                "coordinator", "test identity"), 30)
        for name, bad in (
                ("active", 0), ("main_pid", False),
                ("control_group_absent", 1), ("listener_pids", [False])):
            with self.subTest(stop_field=name):
                candidate = copy.deepcopy(stopped)
                candidate[name] = bad
                with self.assertRaisesRegex(maintenance.MaintenanceError, "field types"):
                    maintenance.validate_stop(
                        candidate,
                        maintenance.parse_identity(
                            self.authorization["production_before"]["coordinator"],
                            "coordinator", "test identity"),
                        "stop proof")
        coordinator = maintenance.parse_identity(
            identity("coordinator", fresh=True), "coordinator", "coordinator")
        worker = maintenance.parse_identity(identity("worker", fresh=True), "worker", "worker")
        probe = self.runner.minimal_two_rank_inference(
            coordinator, worker, self.authorization["recovery_probe"]["request_sha256"], 30)
        for name, bad in (
                ("prompt_tokens", 5.0), ("generated_tokens", True),
                ("world_size", 2.0), ("completed", 1), ("performance_result", 0)):
            with self.subTest(probe_field=name):
                candidate = copy.deepcopy(probe)
                candidate[name] = bad
                with self.assertRaisesRegex(maintenance.MaintenanceError, "field types"):
                    maintenance.validate_probe(
                        candidate,
                        maintenance.load_authorization_bytes(
                            self.auth_path.read_bytes(), expected_sha256=digest(self.auth_path), now=NOW),
                        coordinator, worker)

    def test_recovery_probe_authorization_is_exactly_bounded(self) -> None:
        for field, value in (
                ("prompt_tokens", 6), ("generated_tokens", 2),
                ("prompt_tokens", True), ("generated_tokens", False),
                ("world_size", 2.0), ("world_size", True)):
            with self.subTest(field=field, value=value):
                changed = copy.deepcopy(self.authorization)
                changed["recovery_probe"][field] = value
                content = json.dumps(changed, sort_keys=True).encode("utf-8")
                with self.assertRaises(maintenance.MaintenanceError):
                    maintenance.load_authorization_bytes(
                        content, expected_sha256=hashlib.sha256(content).hexdigest(), now=NOW)

    def test_missing_final_commit_marker_never_publishes_offline_success(self) -> None:
        with mock.patch.object(
                maintenance.EvidenceCustody, "finalize_hashes",
                side_effect=OSError("synthetic manifest finalization failure")):
            with self.assertRaises(maintenance.MaintenanceRunFailed):
                self.execute()
        terminal = json.loads((self.evidence / "terminal.json").read_text(encoding="utf-8"))
        self.assertEqual(terminal["status"], "failure")
        self.assertTrue(any(
            item["stage"] == "evidence finalization" for item in terminal["errors"]))
        self.assertFalse((self.evidence / "SHA256SUMS").exists())
        self.assertFalse((self.evidence / "COMMITTED.json").exists())
        self.assertFalse((self.evidence / "COMMITTING.json").exists())

    def test_terminal_rewrite_failure_still_cannot_publish_success(self) -> None:
        with mock.patch.object(
                maintenance.EvidenceCustody, "finalize_hashes",
                side_effect=OSError("synthetic manifest failure")), mock.patch.object(
                maintenance.EvidenceCustody, "write_terminal_failure_after_finalize_error",
                side_effect=OSError("synthetic terminal rewrite failure")):
            failure = self.failure()
        self.assertTrue(any(
            item["stage"] == "failure terminal rewrite" for item in failure.errors))
        self.assertFalse((self.evidence / "COMMITTED.json").exists())
        with self.assertRaises(maintenance.MaintenanceError):
            maintenance.verify_committed_bundle(self.evidence)

    def test_post_rename_sync_failure_withdraws_success_marker(self) -> None:
        original_sync = maintenance.EvidenceCustody._sync_parent

        def fail_committed_sync(path: Path) -> None:
            if path.name == "COMMITTED.json" and path.exists():
                raise OSError("synthetic post-rename directory sync failure")
            original_sync(path)

        with mock.patch.object(
                maintenance.EvidenceCustody, "_sync_parent",
                new=staticmethod(fail_committed_sync)):
            failure = self.failure()
        self.assertFalse((self.evidence / "COMMITTED.json").exists())
        terminal = json.loads(failure.terminal_path.read_text(encoding="utf-8"))
        self.assertEqual(terminal["status"], "failure")

    def test_commit_staging_failure_rewrites_terminal_and_publishes_no_success(self) -> None:
        original_write = maintenance.EvidenceCustody._write_bytes

        def fail_commit_staging(
            custody: maintenance.EvidenceCustody, path: Path, content: bytes,
        ) -> None:
            if path.name == "COMMITTING.json":
                raise OSError("synthetic commit marker staging failure")
            original_write(custody, path, content)

        with mock.patch.object(
                maintenance.EvidenceCustody, "_write_bytes", new=fail_commit_staging):
            failure = self.failure()
        terminal = json.loads(failure.terminal_path.read_text(encoding="utf-8"))
        self.assertEqual(terminal["status"], "failure")
        self.assertFalse((self.evidence / "COMMITTED.json").exists())
        self.assertFalse((self.evidence / "COMMITTING.json").exists())

    def test_commit_rename_failure_before_effect_publishes_no_success(self) -> None:
        original_replace = os.replace

        def fail_before_effect(source: Any, destination: Any) -> None:
            if Path(destination).name == "COMMITTED.json":
                raise OSError("synthetic rename failure before effect")
            original_replace(source, destination)

        with mock.patch.object(os, "replace", new=fail_before_effect):
            terminal = json.loads(self.failure().terminal_path.read_text(encoding="utf-8"))
        self.assertEqual(terminal["status"], "failure")
        self.assertFalse((self.evidence / "COMMITTED.json").exists())

    def test_commit_rename_lost_response_withdraws_success_before_rewrite(self) -> None:
        original_replace = os.replace

        def fail_after_effect(source: Any, destination: Any) -> None:
            if Path(destination).name == "COMMITTED.json":
                original_replace(source, destination)
                raise OSError("synthetic rename response lost after effect")
            original_replace(source, destination)

        with mock.patch.object(os, "replace", new=fail_after_effect):
            terminal = json.loads(self.failure().terminal_path.read_text(encoding="utf-8"))
        self.assertEqual(terminal["status"], "failure")
        self.assertFalse((self.evidence / "COMMITTED.json").exists())

    def test_commit_rename_lost_response_with_ambiguous_withdrawal_is_immutable(self) -> None:
        original_replace = os.replace
        original_unlink = Path.unlink

        def fail_after_effect(source: Any, destination: Any) -> None:
            if Path(destination).name == "COMMITTED.json":
                original_replace(source, destination)
                raise OSError("synthetic rename response lost after effect")
            original_replace(source, destination)

        def fail_committed_unlink(path: Path, *args: Any, **kwargs: Any) -> None:
            if path.name == "COMMITTED.json":
                raise OSError("synthetic marker withdrawal failure")
            original_unlink(path, *args, **kwargs)

        with mock.patch.object(os, "replace", new=fail_after_effect), mock.patch.object(
                Path, "unlink", new=fail_committed_unlink):
            failure = self.failure()
        self.assertTrue((self.evidence / "COMMITTED.json").exists())
        terminal = json.loads(failure.terminal_path.read_text(encoding="utf-8"))
        self.assertEqual(terminal["status"], "success")
        self.assertEqual(maintenance.verify_committed_bundle(self.evidence)["status"], "success")

    def test_ambiguous_marker_withdrawal_never_mutates_bound_terminal(self) -> None:
        original_sync = maintenance.EvidenceCustody._sync_parent
        original_unlink = Path.unlink

        def fail_committed_sync(path: Path) -> None:
            if path.name == "COMMITTED.json":
                raise OSError("synthetic post-rename directory sync failure")
            original_sync(path)

        def fail_committed_unlink(path: Path, *args: Any, **kwargs: Any) -> None:
            if path.name == "COMMITTED.json":
                raise OSError("synthetic marker withdrawal failure")
            original_unlink(path, *args, **kwargs)

        with mock.patch.object(
                maintenance.EvidenceCustody, "_sync_parent",
                new=staticmethod(fail_committed_sync)), mock.patch.object(
                Path, "unlink", new=fail_committed_unlink):
            failure = self.failure()
        self.assertTrue((self.evidence / "COMMITTED.json").exists())
        terminal = json.loads(failure.terminal_path.read_text(encoding="utf-8"))
        self.assertEqual(terminal["status"], "success")
        self.assertEqual(maintenance.verify_committed_bundle(self.evidence)["status"], "success")

    def test_commit_verifier_rejects_manifest_disagreement_and_extra_files(self) -> None:
        self.execute()
        terminal_path = self.evidence / "terminal.json"
        terminal = json.loads(terminal_path.read_text(encoding="utf-8"))
        terminal["authorization_id"] += "-tampered"
        write_json(terminal_path, terminal)
        with self.assertRaisesRegex(maintenance.MaintenanceError, "terminal.json"):
            maintenance.verify_committed_bundle(self.evidence)

        self.tearDown()
        self.setUp()
        self.execute()
        write_json(self.evidence / "unexpected.json", {"unexpected": True})
        with self.assertRaisesRegex(maintenance.MaintenanceError, "inventory"):
            maintenance.verify_committed_bundle(self.evidence)

    def test_commit_verifier_rejects_semantic_tamper_after_hashes_are_rebuilt(self) -> None:
        self.execute()
        receipt_path = self.evidence / "adapter-receipt.raw.json"
        receipt = json.loads(receipt_path.read_text(encoding="utf-8"))
        receipt["performance_claim"] = True
        receipt_path.write_bytes(maintenance._pretty_json_bytes(receipt))
        handoff_path = self.evidence / maintenance.event_relative_path(9, "adapter handoff")
        handoff = json.loads(handoff_path.read_text(encoding="utf-8"))
        handoff["observation"]["receipt_sha256"] = digest(receipt_path)
        handoff_path.write_bytes(maintenance._pretty_json_bytes(handoff))
        rebuild_unsigned_bundle_bindings(self.evidence)
        with self.assertRaisesRegex(
                maintenance.MaintenanceError, "retained PR51 adapter receipt"):
            maintenance.verify_committed_bundle(self.evidence)

    def test_commit_verifier_rejects_rehashed_policy_path_traversal(self) -> None:
        self.execute()
        policy_path = self.evidence / "policy.raw.json"
        policy = json.loads(policy_path.read_text(encoding="utf-8"))
        policy["incident"]["manifest_path"] = "../../evil-incident.json"
        policy["adapter"]["plan_path"] = "../../evil-plan.json"
        policy["adapter"]["policy_path"] = "../../evil-policy.json"
        policy_path.write_bytes(maintenance._pretty_json_bytes(policy))
        policy_digest = digest(policy_path)
        for relative in ("terminal.json", "intent.json"):
            path = self.evidence / relative
            value = json.loads(path.read_text(encoding="utf-8"))
            value["policy_sha256"] = policy_digest
            path.write_bytes(maintenance._pretty_json_bytes(value))
        rebuild_unsigned_bundle_bindings(self.evidence)
        with self.assertRaisesRegex(
                maintenance.MaintenanceError, "canonical repository-relative path"):
            maintenance.verify_committed_bundle(self.evidence)

    def test_mutation_between_manifest_and_marker_is_unpublished(self) -> None:
        original_commit = maintenance.EvidenceCustody.commit_success

        def mutate_then_commit(
            custody: maintenance.EvidenceCustody, terminal_path: Path, hashes_path: Path,
        ) -> Path:
            terminal = json.loads(terminal_path.read_text(encoding="utf-8"))
            terminal["authorization_id"] += "-between-finalize-and-commit"
            write_json(terminal_path, terminal)
            return original_commit(custody, terminal_path, hashes_path)

        with mock.patch.object(
                maintenance.EvidenceCustody, "commit_success", new=mutate_then_commit):
            failure = self.failure()
        self.assertTrue(any(
            item["stage"] == "committed bundle verification" for item in failure.errors))
        self.assertFalse((self.evidence / "COMMITTED.json").exists())

    def test_coordinator_recovery_failure_is_terminal(self) -> None:
        self.runner.fail_coordinator_start = True
        failure = self.failure()
        terminal = json.loads(failure.terminal_path.read_text(encoding="utf-8"))
        self.assertFalse(terminal["recovery_complete"])
        self.assertNotIn("probe", self.runner.history)

    def test_coordinator_start_error_after_effect_is_reconciled_but_terminal_fails(self) -> None:
        self.runner.fail_coordinator_start_after_effect = True
        failure = self.failure()
        terminal = json.loads(failure.terminal_path.read_text(encoding="utf-8"))
        self.assertEqual(terminal["status"], "failure")
        self.assertTrue(terminal["recovery_complete"])
        self.assertTrue(terminal["production_final_observed"]["roles"]["coordinator"]["active"])
        self.assertIn("coordinator", terminal["production_recovered"])
        self.assertTrue(any(
            item["stage"] == "coordinator recovery actuation" for item in terminal["errors"]))
        self.assertIn("snapshot", self.runner.history)

    def test_worker_start_error_after_effect_remains_worker_first_and_recovers(self) -> None:
        self.runner.fail_worker_start_after_effect = True
        failure = self.failure()
        terminal = json.loads(failure.terminal_path.read_text(encoding="utf-8"))
        self.assertEqual(terminal["status"], "failure")
        self.assertTrue(terminal["recovery_complete"])
        self.assertLess(
            self.runner.history.index("start-worker"),
            self.runner.history.index("start-coordinator"))
        self.assertTrue(any(
            item["stage"] == "worker recovery actuation" for item in terminal["errors"]))

    def test_stop_lost_response_is_reconciled_before_worker_first_recovery(self) -> None:
        for role in ("coordinator", "worker"):
            with self.subTest(role=role):
                self.tearDown()
                self.setUp()
                self.runner.fail_stop_after_effect_role = role
                failure = self.failure()
                terminal = json.loads(failure.terminal_path.read_text(encoding="utf-8"))
                self.assertTrue(terminal["recovery_complete"])
                if role == "coordinator":
                    self.assertNotIn("stop-worker", self.runner.history)
                    self.assertNotIn("start-worker", self.runner.history)
                    self.assertLess(
                        self.runner.history.index("ready-worker"),
                        self.runner.history.index("start-coordinator"))
                else:
                    self.assertLess(
                        self.runner.history.index("start-worker"),
                        self.runner.history.index("start-coordinator"))

    def test_stop_lost_response_with_cgroup_residue_is_not_absence(self) -> None:
        self.runner.fail_stop_after_effect_role = "coordinator"
        self.runner.residual_control_group_role = "coordinator"
        terminal = json.loads(self.failure().terminal_path.read_text(encoding="utf-8"))
        self.assertNotIn("stop-worker", self.runner.history)
        self.assertNotIn("adapter", self.runner.history)
        self.assertFalse(terminal["recovery_complete"])
        self.assertTrue(any(
            item["stage"] == "maintenance body" and "absence state is not exact" in item["detail"]
            for item in terminal["errors"]))

    def test_final_snapshot_represents_inactive_service_and_refuses_success(self) -> None:
        self.runner.drop_role_on_snapshot_call = ("coordinator", 7)
        failure = self.failure()
        terminal = json.loads(failure.terminal_path.read_text(encoding="utf-8"))
        coordinator = terminal["production_final_observed"]["roles"]["coordinator"]
        self.assertEqual(coordinator, {
            "active": False, "host": "nimo-1",
            "unit": maintenance.PROTECTED_UNITS["coordinator"],
            "unit_active_state": "inactive", "unit_sub_state": "dead",
            "main_pid": 0, "listener_pids": [],
            "control_group": f"/system.slice/{maintenance.PROTECTED_UNITS['coordinator']}",
            "control_group_exists": False, "control_group_pids": [],
            "identity": None})
        self.assertFalse(terminal["recovery_complete"])
        self.assertFalse((self.evidence / "COMMITTED.json").exists())
        self.assertTrue((self.evidence / "FAILED.json").exists())

    def test_health_is_not_a_substitute_for_two_rank_inference_contract(self) -> None:
        self.runner.bad_probe = True
        failure = self.failure()
        terminal = json.loads(failure.terminal_path.read_text(encoding="utf-8"))
        self.assertTrue(terminal["services_ready"])
        self.assertTrue(terminal["recovery_census_complete"])
        self.assertFalse(terminal["recovery_probe_complete"])
        self.assertFalse(terminal["recovery_complete"])
        self.assertTrue(any(item["stage"] == "minimal two-rank inference contract" for item in terminal["errors"]))
        self.assertFalse((self.evidence / "COMMITTED.json").exists())
        self.assertTrue((self.evidence / "FAILED.json").exists())

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

    def test_cli_verify_bundle_is_the_success_acceptance_seam(self) -> None:
        self.execute()
        result = subprocess.run([
            sys.executable, str(SOURCE), "--evidence-root", str(self.evidence),
            "verify-bundle",
        ], capture_output=True, text=True, check=False)
        self.assertEqual(result.returncode, 0, result.stderr)
        self.assertTrue(json.loads(result.stdout)["recovery_complete"])
        write_json(self.evidence / "unexpected.json", {"unexpected": True})
        refused = subprocess.run([
            sys.executable, str(SOURCE), "--evidence-root", str(self.evidence),
            "verify-bundle",
        ], capture_output=True, text=True, check=False)
        self.assertEqual(refused.returncode, 1)
        self.assertIn("inventory", refused.stderr)

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
