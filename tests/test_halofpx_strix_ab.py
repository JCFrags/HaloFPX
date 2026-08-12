from __future__ import annotations

import copy
import hashlib
import importlib.util
import json
import tempfile
import unittest
from pathlib import Path


MODULE_PATH = Path(__file__).parents[1] / "scripts" / "halofpx_strix_ab.py"
SPEC = importlib.util.spec_from_file_location("halofpx_strix_ab", MODULE_PATH)
assert SPEC and SPEC.loader
AB = importlib.util.module_from_spec(SPEC)
SPEC.loader.exec_module(AB)


def sha(path: Path) -> str:
    return hashlib.sha256(path.read_bytes()).hexdigest()


class StrixABTest(unittest.TestCase):
    def setUp(self) -> None:
        self.temporary = tempfile.TemporaryDirectory()
        self.root = Path(self.temporary.name)
        self.files = {}
        for name in ("model", "request", "nimo1", "nimo2", "off-server", "on-server", "off-worker", "on-worker"):
            path = self.root / name
            path.write_bytes((name + "\n").encode())
            self.files[name] = path
        self.plan = {
            "schema": AB.PLAN_SCHEMA,
            "experiment_id": "test-model-general",
            "issues": [15, 16],
            "source": {"repository": "https://github.com/JCFrags/HaloFPX.git", "off_commit": "0" * 40, "on_commit": "1" * 40},
            "model": {"path": str(self.files["model"]), "sha256": sha(self.files["model"]), "size_bytes": self.files["model"].stat().st_size, "format_family": "rocmfpx", "architecture": "arbitrary-dense"},
            "request": {"path": str(self.files["request"]), "sha256": sha(self.files["request"]), "prompt_tokens": 512, "output_tokens": 8, "require_content_parity": True, "expected_content_sha256": hashlib.sha256(b"same").hexdigest()},
            "topology": {
                "world_size": 2,
                "rpc_endpoint": "nimo-2:50052",
                "coordinator": {"host": "nimo-1", "device": "ROCm0", "authority_receipt": {"path": str(self.files["nimo1"]), "sha256": sha(self.files["nimo1"])}},
                "worker": {"host": "nimo-2", "device": "ROCm0", "authority_receipt": {"path": str(self.files["nimo2"]), "sha256": sha(self.files["nimo2"])}},
            },
            "runtime": {"lane": "cold_prompt_generation", "cache_class": "cold_cache_off", "context": 1024, "batch": 512, "ubatch": 512, "flash_attention": True, "kv_k": "q8_0", "kv_v": "q8_0", "common_environment": {"HSA_ENABLE_SDMA": "0"}, "common_worker_args": ["--port", "50052"], "common_coordinator_args": ["--port", "18080", "--model", str(self.files["model"]), "--rpc", "nimo-2:50052", "--ctx-size", "1024", "--batch-size", "512", "--ubatch-size", "512", "--cache-type-k", "q8_0", "--cache-type-v", "q8_0", "--flash-attn", "on"]},
            "execution": {"pairs": 3, "order_seed": 7, "warmups_per_condition": 1, "retained_per_condition_per_pair": 1, "profiling_separate": True},
            "conditions": {
                "off": {"source_commit": "0" * 40, "coordinator_binary": {"path": str(self.files["off-server"]), "sha256": sha(self.files["off-server"])}, "worker_binary": {"path": str(self.files["off-worker"]), "sha256": sha(self.files["off-worker"])}, "coordinator_args": [], "worker_args": []},
                "on": {"source_commit": "1" * 40, "coordinator_binary": {"path": str(self.files["on-server"]), "sha256": sha(self.files["on-server"])}, "worker_binary": {"path": str(self.files["on-worker"]), "sha256": sha(self.files["on-worker"])}, "coordinator_args": [], "worker_args": []},
            },
        }

    def tearDown(self) -> None:
        self.temporary.cleanup()

    def initialize(self) -> Path:
        plan_path = self.root / "plan.json"
        plan_path.write_text(json.dumps(self.plan), encoding="utf-8")
        run = self.root / "run"
        AB.init_run(plan_path, run)
        return run

    def import_preflights(self, run: Path) -> None:
        for role, host in (("coordinator", "nimo-1"), ("worker", "nimo-2")):
            receipt = AB.collect_preflight(self.plan, role, observed_hostname=host)
            path = self.root / f"{role}.json"
            AB.write_json(path, receipt)
            AB.import_preflight(run, path)

    def raw_success(self, prompt_tps: float, generation_tps: float, wall_ms: float) -> tuple[Path, Path]:
        response = self.root / f"response-{prompt_tps}-{generation_tps}.json"
        response.write_text(json.dumps({"content": "same", "timings": {"prompt_n": 512, "predicted_n": 8, "prompt_ms": 512 / prompt_tps * 1000, "predicted_ms": 8 / generation_tps * 1000, "prompt_per_second": prompt_tps, "predicted_per_second": generation_tps}}), encoding="utf-8")
        client = self.root / f"client-{wall_ms}.json"
        client.write_text(json.dumps({"schema": AB.CLIENT_SCHEMA, "started_at": "2026-08-12T00:00:00Z", "ended_at": "2026-08-12T00:00:01Z", "http_status": 200, "wall_ms": wall_ms, "ttft_ms": wall_ms / 10, "itl_ms": [1000 / generation_tps] * 7}), encoding="utf-8")
        return response, client

    def test_plan_and_schedule_are_model_general_and_balanced(self) -> None:
        AB.validate_plan(copy.deepcopy(self.plan))
        schedule = AB.make_schedule(self.plan)
        self.assertEqual(len(schedule["entries"]), 6)
        for pair in range(1, 4):
            self.assertEqual({entry["condition"] for entry in schedule["entries"] if entry["pair_id"] == pair}, {"off", "on"})

    def test_feature_branches_may_use_empty_condition_arguments(self) -> None:
        plan = copy.deepcopy(self.plan)
        plan["runtime"]["common_worker_args"] = []
        for condition in ("off", "on"):
            plan["conditions"][condition]["worker_args"] = []
            plan["conditions"][condition]["coordinator_args"] = []
        AB.validate_plan(plan)

    def test_plan_rejects_wrong_lane_and_same_host(self) -> None:
        broken = copy.deepcopy(self.plan)
        broken["runtime"]["cache_class"] = "fresh_process_exact_hit"
        with self.assertRaises(AB.PlanError):
            AB.validate_plan(broken)

    def test_plan_rejects_unmatched_condition_runtime_arguments(self) -> None:
        broken = copy.deepcopy(self.plan)
        broken["conditions"]["on"]["coordinator_args"] = ["--batch-size", "1"]
        with self.assertRaises(AB.PlanError):
            AB.validate_plan(broken)
        broken = copy.deepcopy(self.plan)
        broken["topology"]["worker"]["host"] = "nimo-1"
        with self.assertRaises(AB.PlanError):
            AB.validate_plan(broken)

    def test_preflight_rejects_changed_binary(self) -> None:
        self.files["off-worker"].write_text("changed", encoding="utf-8")
        with self.assertRaises(AB.PlanError):
            AB.collect_preflight(self.plan, "worker", observed_hostname="nimo-2")

    def test_import_rejects_edited_preflight_artifact(self) -> None:
        run = self.initialize()
        receipt = AB.collect_preflight(self.plan, "worker", observed_hostname="nimo-2")
        receipt["artifacts"]["off_binary"]["sha256"] = "f" * 64
        path = self.root / "edited-worker.json"
        AB.write_json(path, receipt)
        with self.assertRaises(AB.PlanError):
            AB.import_preflight(run, path)

    def test_record_rejects_edited_schedule(self) -> None:
        run = self.initialize()
        schedule = json.loads((run / "schedule.json").read_text(encoding="utf-8"))
        schedule["entries"].reverse()
        AB.write_json(run / "schedule.json", schedule)
        first = schedule["entries"][0]
        response, client = self.raw_success(100.0, 20.0, 1000.0)
        with self.assertRaises(AB.PlanError):
            AB.record_sample(run, first["pair_id"], first["condition"], first["order_index"], response, client, "success", None, [])

    def test_complete_paired_analysis_uses_pairs(self) -> None:
        run = self.initialize()
        self.import_preflights(run)
        schedule = json.loads((run / "schedule.json").read_text(encoding="utf-8"))
        for entry in schedule["entries"]:
            on = entry["condition"] == "on"
            response, client = self.raw_success(110.0 if on else 100.0, 22.0 if on else 20.0, 900.0 if on else 1000.0)
            AB.record_sample(run, entry["pair_id"], entry["condition"], entry["order_index"], response, client, "success", None, [])
        report = AB.analyze_run(run)
        self.assertTrue(report["complete"])
        self.assertFalse(report["performance_claim"])
        self.assertEqual(report["metrics"]["prompt_tokens_per_second"]["pair_count"], 3)
        self.assertAlmostEqual(report["metrics"]["prompt_tokens_per_second"]["paired_improvement_percent_mean"], 10.0)
        self.assertAlmostEqual(report["metrics"]["client_wall_ms"]["paired_improvement_percent_mean"], 10.0)

    def test_content_mismatch_and_incomplete_run_fail_closed(self) -> None:
        run = self.initialize()
        self.import_preflights(run)
        schedule = json.loads((run / "schedule.json").read_text(encoding="utf-8"))
        first = schedule["entries"][0]
        response, client = self.raw_success(100.0, 20.0, 1000.0)
        AB.record_sample(run, first["pair_id"], first["condition"], first["order_index"], response, client, "success", None, [])
        report = AB.analyze_run(run)
        self.assertFalse(report["complete"])
        self.assertTrue(report["missing"])

    def test_invalid_raw_sample_does_not_reserve_schedule_slot(self) -> None:
        run = self.initialize()
        first = json.loads((run / "schedule.json").read_text(encoding="utf-8"))["entries"][0]
        response, client = self.raw_success(100.0, 20.0, 1000.0)
        response.write_text(json.dumps({"content": "wrong", "timings": {}}), encoding="utf-8")
        with self.assertRaises(AB.PlanError):
            AB.record_sample(run, first["pair_id"], first["condition"], first["order_index"], response, client, "success", None, [])
        destination = run / "raw" / f"pair-{first['pair_id']:03d}-order-{first['order_index']}-{first['condition']}"
        self.assertFalse(destination.exists())

    def test_analysis_rejects_raw_evidence_modified_after_record(self) -> None:
        run = self.initialize()
        self.import_preflights(run)
        first = json.loads((run / "schedule.json").read_text(encoding="utf-8"))["entries"][0]
        response, client = self.raw_success(100.0, 20.0, 1000.0)
        AB.record_sample(run, first["pair_id"], first["condition"], first["order_index"], response, client, "success", None, [])
        destination = run / "raw" / f"pair-{first['pair_id']:03d}-order-{first['order_index']}-{first['condition']}"
        (destination / "response.json").write_text("{}", encoding="utf-8")
        with self.assertRaises(AB.PlanError):
            AB.analyze_run(run)


if __name__ == "__main__":
    unittest.main()
