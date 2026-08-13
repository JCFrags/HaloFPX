from __future__ import annotations

import copy
import datetime as dt
import hashlib
import importlib.util
import json
import tempfile
import time
import unittest
import sys
from unittest import mock
from pathlib import Path
from typing import Any, Sequence


ROOT = Path(__file__).parents[1]


def load_module(name: str, path: Path):
    spec = importlib.util.spec_from_file_location(name, path)
    assert spec and spec.loader
    module = importlib.util.module_from_spec(spec)
    sys.modules[name] = module
    spec.loader.exec_module(module)
    return module


AB = load_module("halofpx_strix_ab", ROOT / "scripts" / "halofpx_strix_ab.py")
AD = load_module("halofpx_strix_ab_cachyos", ROOT / "scripts" / "halofpx_strix_ab_cachyos.py")


def sha(path: Path) -> str:
    return hashlib.sha256(path.read_bytes()).hexdigest()


class FakeRunner:
    def __init__(self, request_bytes: bytes):
        self.request_bytes = request_bytes
        self.production = {
            "coordinator": {"host": "nimo-1", "unit": AD.PROTECTED_UNITS["coordinator"], "active": False, "pid": 0},
            "worker": {"host": "nimo-2", "unit": AD.PROTECTED_UNITS["worker"], "active": False, "pid": 0},
        }
        self.units: dict[tuple[str, str], AD.UnitIdentity] = {}
        self.port_map: dict[tuple[str, int], int] = {}
        self.next_pid = 4100
        self.start_count = 0
        self.stop_attempts: list[str] = []
        self.stop_roles: list[str] = []
        self.cleanup_attempts: list[tuple[str, str]] = []
        self.sent_bodies: list[bytes] = []
        self.gpu_pids: dict[str, list[int]] = {"nimo-1": [], "nimo-2": []}
        self.gpu_sequences: dict[str, list[list[int]]] = {}
        self.gpu_calls: dict[str, int] = {"nimo-1": 0, "nimo-2": 0}
        self.identity_pid_override: int | None = None
        self.argv_mismatch = False
        self.cache_n: Any = 0
        self.stop_failure_roles: set[str] = set()
        self.cleanup_failure_roles: set[str] = set()
        self.change_production_after = False
        self.snapshot_count = 0
        self.started_identities: list[AD.UnitIdentity] = []
        self.coordinator_batch_override: str | None = None
        self.observed_sha256_override: str | None = None

    def snapshot_production(self, protected: dict[str, dict[str, Any]]) -> dict[str, Any]:
        self.snapshot_count += 1
        result = copy.deepcopy(self.production)
        if self.change_production_after and self.snapshot_count > 1:
            result["coordinator"]["pid"] = 9999
        return result

    def artifact(self, host: str, path: str) -> dict[str, Any]:
        file = Path(path)
        return {"path": path, "size_bytes": file.stat().st_size, "sha256": sha(file)}

    def gpu_clients(self, host: str) -> dict[str, Any]:
        call = self.gpu_calls[host]
        self.gpu_calls[host] += 1
        sequence = self.gpu_sequences.get(host)
        if sequence:
            pids = sequence[min(call, len(sequence) - 1)]
        elif self.gpu_pids[host]:
            pids = self.gpu_pids[host]
        else:
            pids = [identity.pid for identity in self.units.values() if identity.host == host]
        return {"complete": True, "pids": list(pids), "errors": []}

    def port_owners(self, host: str, port: int) -> list[int]:
        value = self.port_map.get((host, port))
        return [] if value is None else [value]

    def ensure_unit_absent(self, host: str, unit: str) -> None:
        if (host, unit) in self.units:
            raise AD.AdapterError("unit exists")

    def start_unit(
        self, role: str, host: str, unit: str, argv: Sequence[str], environment: dict[str, str],
        executable_sha256: str, port: int, runtime_max_seconds: int, identity_timeout_seconds: int,
        stop_timeout_seconds: int,
    ) -> AD.UnitIdentity:
        self.start_count += 1
        pid = self.identity_pid_override if self.identity_pid_override is not None else self.next_pid
        self.next_pid += 1
        observed_argv_list = list(argv)
        if role == "coordinator" and self.coordinator_batch_override is not None:
            batch_index = observed_argv_list.index("--batch-size")
            observed_argv_list[batch_index + 1] = self.coordinator_batch_override
        observed_argv = tuple(observed_argv_list) + (("--unexpected",) if self.argv_mismatch else ())
        identity = AD.UnitIdentity(
            role, host, unit, pid, f"{self.start_count:032x}", 10000 + self.start_count,
            20000 + self.start_count, f"cursor-{self.start_count}", observed_argv,
            dict(environment), self.observed_sha256_override or executable_sha256, port,
            f"/user.slice/user-1000.slice/user@1000.service/app.slice/{unit}")
        self.units[(host, unit)] = identity
        self.port_map[(host, port)] = pid
        self.started_identities.append(identity)
        return identity

    def prove_live(self, identity: AD.UnitIdentity, require_listener: bool = True) -> dict[str, Any]:
        if self.units.get((identity.host, identity.unit)) != identity:
            raise AD.AdapterError("identity not live")
        return {"pid": identity.pid, "invocation_id": identity.invocation_id,
                "argv": list(identity.argv), "environment": identity.environment,
                "executable_sha256": identity.executable_sha256}

    def wait_ready(self, identity: AD.UnitIdentity, timeout_seconds: int) -> dict[str, Any]:
        self.prove_live(identity)
        return {"ready": True, "role": identity.role}

    def request(self, host: str, port: int, body: bytes, output_tokens: int, timeout_seconds: int) -> AD.RequestCapture:
        self.sent_bodies.append(body)
        started = time.monotonic_ns()
        time.sleep(0.2)
        ended = time.monotonic_ns()
        prompt_tps, generation_tps = 100.0, 20.0
        response = {
            "content": "same",
            "timings": {
                "cache_n": self.cache_n,
                "prompt_n": 512,
                "predicted_n": output_tokens,
                "prompt_ms": 512 / prompt_tps * 1000,
                "predicted_ms": output_tokens / generation_tps * 1000,
                "prompt_per_second": prompt_tps,
                "predicted_per_second": generation_tps,
            },
        }
        now = dt.datetime.now(dt.timezone.utc)
        client = {
            "schema": AB.CLIENT_SCHEMA,
            "started_at": now.isoformat(),
            "ended_at": (now + dt.timedelta(seconds=1)).isoformat(),
            "http_status": 200,
            "wall_ms": 1000.0,
            "ttft_ms": 100.0,
            "itl_ms": [10.0] * (output_tokens - 1),
        }
        raw = json.dumps(response, sort_keys=True).encode()
        return AD.RequestCapture(response, client, raw, hashlib.sha256(body).hexdigest(), started, ended)

    def telemetry(self, host: str) -> dict[str, Any]:
        return {"boot_id": f"boot-{host}", "monotonic_ns": time.monotonic_ns(), "gpu": {}}

    def stop_unit(self, identity: AD.UnitIdentity, timeout_seconds: int) -> tuple[dict[str, Any], bytes]:
        self.prove_live(identity)
        self.stop_attempts.append(identity.role)
        if identity.role in self.stop_failure_roles:
            raise AD.AdapterError(f"injected {identity.role} terminal failure")
        self.stop_roles.append(identity.role)
        return ({"pid": identity.pid, "invocation_id": identity.invocation_id},
                f"journal {identity.role} {identity.invocation_id}\n".encode())

    def cleanup_unit(
        self, host: str, unit: str, port: int, timeout_seconds: int,
        identity: AD.UnitIdentity | None = None,
    ) -> dict[str, Any]:
        self.cleanup_attempts.append((host, unit))
        identity = self.units.pop((host, unit), None)
        self.port_map.pop((host, port), None)
        role = identity.role if identity else ("coordinator" if "coordinator" in unit else "worker")
        if role in self.cleanup_failure_roles:
            raise AD.AdapterError(f"injected {role} cleanup failure")
        return {
            "unit_absent": True, "port_closed": True,
            "captured_pid_absent": identity is None or identity.pid not in self.port_map.values(),
            "captured_cgroup_absent": True,
            "captured_pid": identity.pid if identity is not None else None,
            "captured_control_group": identity.control_group if identity is not None else None,
            "identity_source": "provided" if identity is not None else "none",
        }


class AdapterTest(unittest.TestCase):
    def setUp(self) -> None:
        self.temporary = tempfile.TemporaryDirectory()
        self.root = Path(self.temporary.name)
        self.files: dict[str, Path] = {}
        for name in ("model", "nimo1", "nimo2", "off-server", "on-server", "off-worker", "on-worker"):
            path = self.root / name
            path.write_bytes((name + "\n").encode())
            self.files[name] = path
        request = json.dumps({
            "prompt": "frozen prompt", "n_predict": 8, "stream": True,
            "cache_prompt": False, "ignore_eos": True, "seed": 1234, "temperature": 0,
        }, separators=(",", ":")).encode()
        self.files["request"] = self.root / "request.json"
        self.files["request"].write_bytes(request)
        self.plan = {
            "schema": AB.PLAN_SCHEMA,
            "experiment_id": "issue37-adapter-test",
            "issues": [15, 16],
            "source": {"repository": "https://github.com/JCFrags/HaloFPX.git", "off_commit": "0" * 40, "on_commit": "1" * 40},
            "model": {"path": str(self.files["model"]), "sha256": sha(self.files["model"]), "size_bytes": self.files["model"].stat().st_size, "format_family": "rocmfpx", "architecture": "arbitrary-dense"},
            "request": {"path": str(self.files["request"]), "sha256": sha(self.files["request"]), "prompt_tokens": 512, "output_tokens": 8, "require_content_parity": True, "expected_content_sha256": hashlib.sha256(b"same").hexdigest()},
            "topology": {
                "world_size": 2, "rpc_endpoint": "nimo-2:50252",
                "coordinator": {"host": "nimo-1", "device": "ROCm0", "authority_receipt": {"path": str(self.files["nimo1"]), "sha256": sha(self.files["nimo1"])}},
                "worker": {"host": "nimo-2", "device": "ROCm0", "authority_receipt": {"path": str(self.files["nimo2"]), "sha256": sha(self.files["nimo2"])}},
            },
            "runtime": {
                "lane": "cold_prompt_generation", "cache_class": "cold_cache_off", "context": 1024,
                "batch": 512, "ubatch": 512, "flash_attention": True, "kv_k": "q8_0", "kv_v": "q8_0",
                "common_environment": {"HSA_ENABLE_SDMA": "0"},
                "common_worker_args": ["--host", "0.0.0.0", "--port", "50252"],
                "common_coordinator_args": ["--host", "127.0.0.1", "--port", "18080", "--model", str(self.files["model"]), "--rpc", "nimo-2:50252", "--ctx-size", "1024", "--batch-size", "512", "--ubatch-size", "512", "--cache-type-k", "q8_0", "--cache-type-v", "q8_0", "--flash-attn", "on"],
            },
            "execution": {"pairs": 3, "order_seed": 7, "warmups_per_condition": 1, "retained_per_condition_per_pair": 1, "profiling_separate": True},
            "conditions": {
                "off": {"source_commit": "0" * 40, "coordinator_binary": {"path": str(self.files["off-server"]), "sha256": sha(self.files["off-server"])}, "worker_binary": {"path": str(self.files["off-worker"]), "sha256": sha(self.files["off-worker"])}, "coordinator_args": [], "worker_args": []},
                "on": {"source_commit": "1" * 40, "coordinator_binary": {"path": str(self.files["on-server"]), "sha256": sha(self.files["on-server"])}, "worker_binary": {"path": str(self.files["on-worker"]), "sha256": sha(self.files["on-worker"])}, "coordinator_args": [], "worker_args": []},
            },
        }
        plan_path = self.root / "plan-input.json"
        plan_path.write_text(json.dumps(self.plan), encoding="utf-8")
        self.run = self.root / "run"
        AB.init_run(plan_path, self.run)
        for role, host in (("coordinator", "nimo-1"), ("worker", "nimo-2")):
            receipt = AB.collect_preflight(self.plan, role, observed_hostname=host)
            receipt_path = self.root / f"{role}.json"
            AB.write_json(receipt_path, receipt)
            AB.import_preflight(self.run, receipt_path)
        self.policy = {
            "schema": AD.POLICY_SCHEMA,
            "issue": 37,
            "controller_host": "nimo-1",
            "protected": {
                "coordinator": {"host": "nimo-1", "unit": AD.PROTECTED_UNITS["coordinator"], "ports": [8081], "health_url": "http://127.0.0.1:8081/health"},
                "worker": {"host": "nimo-2", "unit": AD.PROTECTED_UNITS["worker"], "ports": [50052], "health_url": None},
            },
            "disposable": {"unit_prefix": "halofpx-ab-", "coordinator_port": 18080, "worker_port": 50252, "runtime_max_seconds": 3600},
            "timeouts": {"identity_seconds": 10, "readiness_seconds": 60, "request_seconds": 60, "stop_seconds": 10, "telemetry_interval_seconds": 0.1},
            "require_no_foreign_gpu_clients": True,
        }
        self.policy_path = self.root / "policy.json"
        self.policy_path.write_text(json.dumps(self.policy), encoding="utf-8")

    def make_v2_run(self, kind: str = "runtime_n_batch") -> tuple[dict[str, Any], Path]:
        plan = copy.deepcopy(self.plan)
        plan["schema"] = AB.PLAN_SCHEMA_V2
        plan["comparison"] = {"kind": kind, "control": "off", "candidate": "on"}
        plan["runtime"].pop("batch")
        plan["runtime"]["batch_by_condition"] = {
            "off": 512, "on": 2048 if kind == "runtime_n_batch" else 512}
        args = plan["runtime"]["common_coordinator_args"]
        batch_index = args.index("--batch-size")
        del args[batch_index:batch_index + 2]
        plan["execution"]["pairs"] = 1
        if kind == "runtime_n_batch":
            plan["source"]["on_commit"] = plan["source"]["off_commit"]
            plan["conditions"]["on"]["source_commit"] = plan["source"]["off_commit"]
            for key in ("coordinator_binary", "worker_binary"):
                plan["conditions"]["on"][key] = copy.deepcopy(
                    plan["conditions"]["off"][key])
        plan_path = self.root / f"plan-{kind}.json"
        plan_path.write_text(json.dumps(plan), encoding="utf-8")
        run = self.root / f"run-{kind}"
        AB.init_run(plan_path, run)
        for role, host in (("coordinator", "nimo-1"), ("worker", "nimo-2")):
            receipt = AB.collect_preflight(plan, role, observed_hostname=host)
            receipt_path = self.root / f"{kind}-{role}.json"
            AB.write_json(receipt_path, receipt)
            AB.import_preflight(run, receipt_path)
        return plan, run

    def tearDown(self) -> None:
        self.temporary.cleanup()

    def test_happy_path_uses_fresh_warmup_and_measured_processes(self) -> None:
        runner = FakeRunner(self.files["request"].read_bytes())
        receipt_path = AD.execute_next(self.run, self.policy_path, runner)
        receipt = json.loads(receipt_path.read_text(encoding="utf-8"))
        self.assertEqual(receipt["outcome"]["status"], "success")
        self.assertEqual([cycle["kind"] for cycle in receipt["cycles"]], ["warmup", "measurement"])
        self.assertEqual(runner.start_count, 4)
        self.assertEqual(runner.stop_roles, ["coordinator", "worker", "coordinator", "worker"])
        self.assertEqual(runner.sent_bodies, [self.files["request"].read_bytes()] * 2)
        self.assertEqual(set(receipt["cycles"][1]["telemetry"]), {"nimo-1", "nimo-2"})
        self.assertGreater(receipt["cycles"][1]["terminal"]["worker"]["journal"]["size_bytes"], 0)
        self.assertTrue(receipt["cycles"][1]["cleanup"]["worker"]["captured_pid_absent"])
        measurement = receipt["cycles"][1]
        samples = measurement["gpu_admission"]["samples"]
        self.assertGreaterEqual(len(samples), 3)
        self.assertTrue(any(
            row["admission"][role]["controller_ended_ns"] <=
            measurement["request"]["controller_started_monotonic_ns"]
            for role in ("coordinator", "worker") for row in samples))
        self.assertTrue(any(
            row["admission"][role]["controller_started_ns"] >=
            measurement["request"]["controller_ended_monotonic_ns"]
            for role in ("coordinator", "worker") for row in samples))
        self.assertTrue(any(
            row["admission"][role]["controller_ended_ns"] >=
            measurement["request"]["controller_started_monotonic_ns"] and
            row["admission"][role]["controller_started_ns"] <=
            measurement["request"]["controller_ended_monotonic_ns"]
            for role in ("coordinator", "worker") for row in samples))
        self.assertFalse(receipt["execution_qualified"])
        raw_samples = list((self.run / "raw").glob("*/sample.json"))
        self.assertEqual(len(raw_samples), 1)
        self.assertEqual(json.loads(raw_samples[0].read_text())["status"], "success")

    def test_global_schedule_is_next_only_and_ambiguous_intent_never_retries(self) -> None:
        runner = FakeRunner(self.files["request"].read_bytes())
        first = AD.execute_next(self.run, self.policy_path, runner)
        self.assertEqual(json.loads(first.read_text())["schedule_index"], 0)
        second = AD.execute_next(self.run, self.policy_path, runner)
        self.assertEqual(json.loads(second.read_text())["schedule_index"], 1)
        schedule = json.loads((self.run / "schedule.json").read_text())
        directory = self.run / "execution" / "entry-002"
        directory.mkdir()
        AD.write_json_durable(directory / "intent.json", {
            "schema": AD.INTENT_SCHEMA, "schedule_index": 2, "entry": schedule["entries"][2]})
        with self.assertRaisesRegex(AD.AdapterError, "never retry"):
            AD.next_schedule_entry(self.run, schedule)

    def test_completed_receipt_tampering_blocks_schedule_progress(self) -> None:
        runner = FakeRunner(self.files["request"].read_bytes())
        receipt_path = AD.execute_next(self.run, self.policy_path, runner)
        receipt = json.loads(receipt_path.read_text())
        receipt["entry"]["condition"] = "tampered"
        receipt_path.write_text(json.dumps(receipt), encoding="utf-8")
        schedule = json.loads((self.run / "schedule.json").read_text())
        with self.assertRaisesRegex(AD.AdapterError, "receipt differs"):
            AD.next_schedule_entry(self.run, schedule, AD.policy_digest(self.policy_path))

    def test_protected_ports_and_units_are_closed_allowlists(self) -> None:
        broken = copy.deepcopy(self.policy)
        broken["disposable"]["worker_port"] = 50052
        path = self.root / "bad-policy.json"
        path.write_text(json.dumps(broken), encoding="utf-8")
        with self.assertRaises(AD.AdapterError):
            AD.load_policy(path, self.plan)
        broken = copy.deepcopy(self.policy)
        broken["protected"]["worker"]["unit"] = "other.service"
        path.write_text(json.dumps(broken), encoding="utf-8")
        with self.assertRaises(AD.AdapterError):
            AD.load_policy(path, self.plan)

    def test_adapter_refuses_provenance_only_ab_without_binary_difference(self) -> None:
        plan = copy.deepcopy(self.plan)
        plan["conditions"]["on"]["coordinator_binary"]["sha256"] = \
            plan["conditions"]["off"]["coordinator_binary"]["sha256"]
        plan["conditions"]["on"]["worker_binary"]["sha256"] = \
            plan["conditions"]["off"]["worker_binary"]["sha256"]
        with self.assertRaisesRegex(AD.AdapterError, "distinct OFF/ON binary|identical binary hashes"):
            AD.load_policy(self.policy_path, plan)

    def test_v2_runtime_batch_accepts_same_binaries_and_executes_exact_generated_argv(self) -> None:
        plan, run = self.make_v2_run()
        policy = AD.load_policy(self.policy_path, plan)
        self.assertEqual(policy.worker_port, 50252)
        commands = AB.commands_document(plan)["conditions"]
        self.assertEqual(commands["off"]["worker"], commands["on"]["worker"])
        self.assertEqual(commands["off"]["coordinator"][-2:], ["--batch-size", "512"])
        self.assertEqual(commands["on"]["coordinator"][-2:], ["--batch-size", "2048"])

        runner = FakeRunner(self.files["request"].read_bytes())
        AD.execute_next(run, self.policy_path, runner)
        AD.execute_next(run, self.policy_path, runner)
        coordinator = [item for item in runner.started_identities if item.role == "coordinator"]
        worker = [item for item in runner.started_identities if item.role == "worker"]
        self.assertEqual(len(coordinator), 4)
        self.assertEqual(len(worker), 4)
        self.assertEqual({item.executable_sha256 for item in coordinator}, {
            plan["conditions"]["off"]["coordinator_binary"]["sha256"]})
        self.assertEqual({item.executable_sha256 for item in worker}, {
            plan["conditions"]["off"]["worker_binary"]["sha256"]})
        by_condition = {
            condition: [item for item in coordinator if f"-{condition}-coordinator" in item.unit]
            for condition in ("off", "on")}
        self.assertEqual(
            {AD.argv_option(item.argv, "--batch-size") for item in by_condition["off"]}, {"512"})
        self.assertEqual(
            {AD.argv_option(item.argv, "--batch-size") for item in by_condition["on"]}, {"2048"})

    def test_v2_runtime_preflight_equal_artifacts_are_bound_and_tampering_refuses(self) -> None:
        plan, run = self.make_v2_run()
        receipt = AB.read_json(run / "preflight" / "coordinator.json")
        self.assertEqual(receipt["artifacts"]["off_binary"], receipt["artifacts"]["on_binary"])
        receipt["artifacts"]["on_binary"]["sha256"] = "f" * 64
        with self.assertRaises(AB.PlanError):
            AB.validate_preflight_receipt(plan, receipt)

    def test_v2_runtime_live_batch_substitution_fails_closed_and_cleans_both_roles(self) -> None:
        _, run = self.make_v2_run()
        runner = FakeRunner(self.files["request"].read_bytes())
        runner.coordinator_batch_override = "4096"
        with self.assertRaises(AD.AdapterError):
            AD.execute_next(run, self.policy_path, runner)
        self.assertEqual(len(runner.cleanup_attempts), 2)
        sample = next((run / "raw").glob("*/sample.json"))
        self.assertEqual(json.loads(sample.read_text())["status"], "failure")

    def test_v2_runtime_live_executable_substitution_fails_closed_and_cleans_both_roles(self) -> None:
        _, run = self.make_v2_run()
        runner = FakeRunner(self.files["request"].read_bytes())
        runner.observed_sha256_override = "f" * 64
        with self.assertRaises(AD.AdapterError):
            AD.execute_next(run, self.policy_path, runner)
        self.assertEqual(len(runner.cleanup_attempts), 2)
        self.assertEqual(runner.start_count, 1)

    def test_v2_feature_build_keeps_adapter_distinct_binary_rule(self) -> None:
        plan = copy.deepcopy(self.plan)
        plan["schema"] = AB.PLAN_SCHEMA_V2
        plan["comparison"] = {"kind": "feature_build", "control": "off", "candidate": "on"}
        plan["runtime"].pop("batch")
        plan["runtime"]["batch_by_condition"] = {"off": 512, "on": 512}
        args = plan["runtime"]["common_coordinator_args"]
        index = args.index("--batch-size")
        del args[index:index + 2]
        for key in ("coordinator_binary", "worker_binary"):
            plan["conditions"]["on"][key]["sha256"] = plan["conditions"]["off"][key]["sha256"]
        with self.assertRaisesRegex(AD.AdapterError, "distinct OFF/ON binary|identical binary hashes"):
            AD.load_policy(self.policy_path, plan)

    def test_v2_target_execution_remains_blocked_before_ssh(self) -> None:
        _, run = self.make_v2_run()
        runner = AD.SshCachyRunner()
        self.assertFalse(AD.TARGET_EXECUTION_ENABLED)
        with mock.patch.object(runner, "_run") as remote, \
                self.assertRaisesRegex(AD.AdapterError, "issue #41"):
            AD.execute_next(run, self.policy_path, runner)
        remote.assert_not_called()

    def test_adapter_refuses_rpc_endpoint_on_wrong_host(self) -> None:
        plan = copy.deepcopy(self.plan)
        plan["topology"]["rpc_endpoint"] = "foreign-host:50252"
        plan["runtime"]["common_coordinator_args"] = [
            "foreign-host:50252" if value == "nimo-2:50252" else value
            for value in plan["runtime"]["common_coordinator_args"]]
        with self.assertRaisesRegex(AD.AdapterError, "endpoint host differs"):
            AD.load_policy(self.policy_path, plan)

    def test_single_node_adapter_mode_is_explicitly_unsupported(self) -> None:
        plan = copy.deepcopy(self.plan)
        plan["topology"]["world_size"] = 1
        with self.assertRaisesRegex(AD.AdapterError, "single-node execution is not implemented"):
            AD.load_policy(self.policy_path, plan)

    def test_real_target_execution_is_disabled_pending_issues_37_and_41(self) -> None:
        authority = AD.load_issue41_authority()
        self.assertEqual(authority["manifest_sha256"], AD.ISSUE41_MANIFEST_SHA256)
        self.assertEqual(authority["target_execution_state"], "blocked")
        self.assertFalse(authority["target_execution_enabled"])
        self.assertEqual(
            authority["unresolved_custody"], list(AD.ISSUE41_UNRESOLVED_CUSTODY))
        with self.assertRaisesRegex(AD.AdapterError, "issue #41"):
            AD.execute_next(self.run, self.policy_path, AD.SshCachyRunner())

    def test_issue41_authority_rejects_a_changed_manifest(self) -> None:
        authority_root = self.root / "changed-authority"
        manifest = authority_root / AD.ISSUE41_MANIFEST_RELATIVE
        manifest.parent.mkdir(parents=True)
        manifest.write_text("{}\n", encoding="utf-8")
        with self.assertRaisesRegex(AD.AdapterError, "differs from the reviewed"):
            AD.load_issue41_authority(authority_root)

    def test_foreign_gpu_process_refuses_before_schedule_intent(self) -> None:
        runner = FakeRunner(self.files["request"].read_bytes())
        runner.gpu_pids["nimo-2"] = [2148915]
        with self.assertRaisesRegex(AD.AdapterError, "foreign GPU clients"):
            AD.execute_next(self.run, self.policy_path, runner)
        self.assertFalse((self.run / "execution").exists() and any((self.run / "execution").iterdir()))
        self.assertEqual(runner.start_count, 0)

    def test_model_mutation_after_preflight_refuses_before_intent(self) -> None:
        runner = FakeRunner(self.files["request"].read_bytes())
        self.files["model"].write_bytes(b"changed model bytes")
        with self.assertRaisesRegex(AD.AdapterError, "model bytes differ"):
            AD.execute_next(self.run, self.policy_path, runner)
        self.assertEqual(runner.start_count, 0)
        self.assertFalse((self.run / "execution").exists() and any((self.run / "execution").iterdir()))

    def test_active_production_refuses_without_stopping_or_launching(self) -> None:
        runner = FakeRunner(self.files["request"].read_bytes())
        runner.production["coordinator"].update({"active": True, "pid": 3027112})
        with self.assertRaisesRegex(AD.AdapterError, "protected production is active"):
            AD.execute_next(self.run, self.policy_path, runner)
        self.assertEqual(runner.start_count, 0)
        self.assertEqual(runner.stop_roles, [])
        self.assertEqual(runner.cleanup_attempts, [])

    def test_production_pid_collision_fails_and_cleanup_covers_both_nodes(self) -> None:
        runner = FakeRunner(self.files["request"].read_bytes())
        runner.production["worker"].update({"active": False, "pid": 2148915})
        runner.identity_pid_override = 2148915
        with self.assertRaises(AD.AdapterError):
            AD.execute_next(self.run, self.policy_path, runner)
        self.assertEqual(len(runner.cleanup_attempts), 2)
        sample = next((self.run / "raw").glob("*/sample.json"))
        self.assertEqual(json.loads(sample.read_text())["status"], "failure")

    def test_live_argv_mismatch_fails_closed(self) -> None:
        runner = FakeRunner(self.files["request"].read_bytes())
        runner.argv_mismatch = True
        with self.assertRaises(AD.AdapterError):
            AD.execute_next(self.run, self.policy_path, runner)
        self.assertEqual(len(runner.cleanup_attempts), 2)

    def test_nonzero_cache_count_consumes_failed_slot(self) -> None:
        runner = FakeRunner(self.files["request"].read_bytes())
        runner.cache_n = 511
        with self.assertRaises(AD.AdapterError):
            AD.execute_next(self.run, self.policy_path, runner)
        receipt = json.loads((self.run / "execution" / "entry-000" / "execution.json").read_text())
        self.assertEqual(receipt["outcome"]["status"], "failure")
        failed = receipt["cycles"][0]
        self.assertEqual(failed["status"], "failure")
        self.assertIn("cache_n == 0", failed["errors"][0]["detail"])
        self.assertEqual(set(failed["terminal"]), {"coordinator", "worker"})
        self.assertEqual(set(failed["cleanup"]), {"coordinator", "worker"})
        self.assertIn("raw_http_sha256", failed["request"])
        with self.assertRaises(AD.AdapterError):
            AD.execute_next(self.run, self.policy_path, runner)

    def test_foreign_gpu_client_arriving_after_measured_request_invalidates_and_is_retained(self) -> None:
        runner = FakeRunner(self.files["request"].read_bytes())
        original_gpu_clients = runner.gpu_clients

        def gpu_clients(host: str) -> dict[str, Any]:
            result = original_gpu_clients(host)
            if host == "nimo-2" and len(runner.sent_bodies) >= 2:
                result["pids"].append(99991)
            return result

        runner.gpu_clients = gpu_clients  # type: ignore[method-assign]
        with self.assertRaisesRegex(AD.AdapterError, "foreign GPU clients"):
            AD.execute_next(self.run, self.policy_path, runner)
        receipt = json.loads((self.run / "execution" / "entry-000" / "execution.json").read_text())
        measured = receipt["cycles"][1]
        self.assertEqual(measured["status"], "failure")
        observed = [
            row["admission"].get("worker", {}).get("foreign_pids", [])
            for row in measured["gpu_admission"]["samples"] if row.get("admission")
        ]
        self.assertIn([99991], observed)
        self.assertEqual(set(measured["cleanup"]), {"coordinator", "worker"})

    def test_ssh_runner_wraps_remote_child_in_independent_watchdog(self) -> None:
        process = mock.Mock()
        process.returncode = 0
        process.communicate.return_value = (b"ok", b"")
        with mock.patch.object(AD.subprocess, "Popen", return_value=process) as popen:
            result = AD.SshCachyRunner()._run("nimo-1", ["true"], timeout=7)
        self.assertEqual(result.stdout, b"ok")
        command = popen.call_args.args[0]
        self.assertIn("ServerAliveInterval=5", command)
        self.assertIn("/usr/bin/timeout", command[-1])
        self.assertIn("--kill-after=5s", command[-1])
        process.communicate.assert_called_once_with(input=None, timeout=22)

    def test_ssh_timeout_terminates_and_reaps_the_local_process_group(self) -> None:
        process = mock.Mock()
        process.pid = 8123
        process.communicate.side_effect = [
            AD.subprocess.TimeoutExpired(cmd="ssh", timeout=22),
            (b"", b""),
        ]
        with mock.patch.object(AD.os, "name", "posix"), \
                mock.patch.object(AD.os, "killpg", create=True) as killpg, \
                mock.patch.object(AD.subprocess, "Popen", return_value=process) as popen:
            with self.assertRaisesRegex(AD.AdapterError, "local SSH custody timed out"):
                AD.SshCachyRunner()._run("nimo-1", ["sleep", "600"], timeout=7)
        self.assertTrue(popen.call_args.kwargs["start_new_session"])
        killpg.assert_called_once_with(8123, AD.signal.SIGTERM)
        self.assertEqual(process.communicate.call_args_list[-1], mock.call(timeout=5))

    def test_ssh_timeout_escalates_the_local_process_group_to_kill(self) -> None:
        process = mock.Mock()
        process.pid = 8124
        process.communicate.side_effect = [
            AD.subprocess.TimeoutExpired(cmd="ssh", timeout=22),
            AD.subprocess.TimeoutExpired(cmd="ssh", timeout=5),
            (b"", b""),
        ]
        with mock.patch.object(AD.os, "name", "posix"), \
                mock.patch.object(AD.signal, "SIGKILL", 9, create=True), \
                mock.patch.object(AD.os, "killpg", create=True) as killpg, \
                mock.patch.object(AD.subprocess, "Popen", return_value=process):
            with self.assertRaisesRegex(AD.AdapterError, "local SSH custody timed out"):
                AD.SshCachyRunner()._run("nimo-1", ["sleep", "600"], timeout=7)
        self.assertEqual(killpg.call_args_list, [
            mock.call(8124, AD.signal.SIGTERM),
            mock.call(8124, 9),
        ])

    def test_cleanup_recovers_started_identity_and_proves_pid_and_cgroup_absence(self) -> None:
        runner = AD.SshCachyRunner()
        unit = "halofpx-ab-test-e000-warmup0-off-worker.service"
        cgroup = f"/user.slice/user-1000.slice/user@1000.service/app.slice/{unit}"
        invocation = "a" * 32
        pre = (
            "LoadState=loaded\nActiveState=active\nMainPID=4242\n"
            f"InvocationID={invocation}\nControlGroup={cgroup}\n"
            f"FragmentPath=/run/user/1000/systemd/transient/{unit}\n"
        ).encode()
        after = (
            "LoadState=not-found\nActiveState=inactive\nMainPID=0\n"
        ).encode()
        runner._run = mock.Mock(side_effect=[  # type: ignore[method-assign]
            AD.CommandResult(0, pre, b""),
            AD.CommandResult(0, b"", b""),
            AD.CommandResult(0, b"", b""),
            AD.CommandResult(0, after, b""),
            AD.CommandResult(0, b"", b""),
            AD.CommandResult(0, b"", b""),
        ])
        runner._process = mock.Mock(return_value={  # type: ignore[method-assign]
            "pid": 4242, "exe": "/bin/true", "exe_sha256": "0" * 64,
            "argv": ["/bin/true"], "environment": None,
            "cgroup": f"0::{cgroup}\n", "process_start_ticks": 99,
        })
        runner.port_owners = mock.Mock(return_value=[])  # type: ignore[method-assign]
        proof = runner.cleanup_unit("nimo-2", unit, 50252, 10)
        self.assertEqual(proof["captured_pid"], 4242)
        self.assertEqual(proof["captured_control_group"], cgroup)
        self.assertEqual(proof["identity_source"], "cleanup-pre-state")
        self.assertTrue(proof["captured_pid_absent"])
        self.assertTrue(proof["captured_cgroup_absent"])

    def test_cleanup_absence_probe_transport_error_is_not_accepted_as_absence(self) -> None:
        runner = AD.SshCachyRunner()
        runner._run = mock.Mock(return_value=AD.CommandResult(255, b"", b"lost SSH"))  # type: ignore[method-assign]
        with self.assertRaisesRegex(AD.AdapterError, "absence probe failed"):
            runner._path_absent("nimo-1", "/proc/4242", "captured PID")

    def test_unified_cgroup_parser_rejects_substrings_and_ambiguous_membership(self) -> None:
        value = "/user.slice/user-1000.slice/app.slice/test.service"
        self.assertEqual(AD.parse_unified_cgroup(f"0::{value}\n", "test"), value)
        for invalid in (
            f"1:name=systemd:{value}\n",
            f"0::{value}-suffix\n0::{value}\n",
            f"prefix 0::{value} suffix\n",
            "/not-a-cgroup-entry\n",
        ):
            with self.subTest(invalid=invalid):
                with self.assertRaises(AD.AdapterError):
                    AD.parse_unified_cgroup(invalid, "test")

    def test_cleanup_failure_attempts_all_targets_and_invalidates_entry(self) -> None:
        runner = FakeRunner(self.files["request"].read_bytes())
        runner.cleanup_failure_roles = {"coordinator"}
        with self.assertRaises(AD.AdapterError):
            AD.execute_next(self.run, self.policy_path, runner)
        self.assertEqual(len(runner.cleanup_attempts), 2)
        receipt = json.loads((self.run / "execution" / "entry-000" / "execution.json").read_text())
        self.assertEqual(receipt["outcome"]["status"], "failure")

    def test_all_terminal_and_cleanup_errors_survive_in_the_failure_receipt(self) -> None:
        runner = FakeRunner(self.files["request"].read_bytes())
        runner.cache_n = 1
        runner.stop_failure_roles = {"coordinator", "worker"}
        runner.cleanup_failure_roles = {"coordinator", "worker"}
        with self.assertRaises(AD.AdapterError):
            AD.execute_next(self.run, self.policy_path, runner)
        self.assertEqual(runner.stop_attempts, ["coordinator", "worker"])
        self.assertEqual(len(runner.cleanup_attempts), 2)
        receipt_path = self.run / "execution" / "entry-000" / "execution.json"
        receipt = json.loads(receipt_path.read_text())
        stages = [item["stage"] for item in receipt["cycles"][0]["errors"]]
        self.assertIn("request evidence validation", stages)
        self.assertIn("coordinator terminal evidence and stop", stages)
        self.assertIn("worker terminal evidence and stop", stages)
        self.assertIn("coordinator cleanup proof", stages)
        self.assertIn("worker cleanup proof", stages)
        self.assertEqual(len(stages), 5)
        self.assertTrue(all(stage in receipt["outcome"]["detail"] for stage in stages))
        sample_path = next((self.run / "raw").glob("*/sample.json"))
        sample = json.loads(sample_path.read_text())
        retained_name = sample["raw"]["extra_0"]["path"]
        self.assertEqual((sample_path.parent / retained_name).read_bytes(), receipt_path.read_bytes())

    def test_changed_production_snapshot_invalidates_successful_cycle(self) -> None:
        runner = FakeRunner(self.files["request"].read_bytes())
        runner.change_production_after = True
        with self.assertRaisesRegex(AD.AdapterError, "protected production authority changed"):
            AD.execute_next(self.run, self.policy_path, runner)
        receipt = json.loads((self.run / "execution" / "entry-000" / "execution.json").read_text())
        self.assertEqual(receipt["outcome"]["failure_code"], "PRODUCTION_RECONCILIATION")


if __name__ == "__main__":
    unittest.main()
