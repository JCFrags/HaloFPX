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
        self.files["request"].write_bytes(json.dumps({
            "prompt": "frozen prompt",
            "n_predict": 8,
            "stream": True,
            "cache_prompt": False,
            "ignore_eos": True,
            "seed": 1234,
            "temperature": 0,
        }, separators=(",", ":")).encode("utf-8"))
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

    def make_v2(self, kind: str = "runtime_n_batch") -> dict:
        plan = copy.deepcopy(self.plan)
        plan["schema"] = AB.PLAN_SCHEMA_V2
        plan["comparison"] = {"kind": kind, "control": "off", "candidate": "on"}
        plan["runtime"].pop("batch")
        plan["runtime"]["batch_by_condition"] = {
            "off": 512,
            "on": 2048 if kind == "runtime_n_batch" else 512,
        }
        args = plan["runtime"]["common_coordinator_args"]
        batch_index = args.index("--batch-size")
        del args[batch_index:batch_index + 2]
        if kind == "runtime_n_batch":
            plan["source"]["on_commit"] = plan["source"]["off_commit"]
            plan["conditions"]["on"]["source_commit"] = plan["source"]["off_commit"]
            for key in ("coordinator_binary", "worker_binary"):
                plan["conditions"]["on"][key] = copy.deepcopy(
                    plan["conditions"]["off"][key])
        return plan

    def import_preflights(self, run: Path) -> None:
        for role, host in (("coordinator", "nimo-1"), ("worker", "nimo-2")):
            receipt = AB.collect_preflight(self.plan, role, observed_hostname=host)
            path = self.root / f"{role}.json"
            AB.write_json(path, receipt)
            AB.import_preflight(run, path)

    def raw_success(self, prompt_tps: float, generation_tps: float, wall_ms: float) -> tuple[Path, Path]:
        response = self.root / f"response-{prompt_tps}-{generation_tps}.json"
        response.write_text(json.dumps({"content": "same", "timings": {"cache_n": 0, "prompt_n": 512, "predicted_n": 8, "prompt_ms": 512 / prompt_tps * 1000, "predicted_ms": 8 / generation_tps * 1000, "prompt_per_second": prompt_tps, "predicted_per_second": generation_tps}}), encoding="utf-8")
        client = self.root / f"client-{wall_ms}.json"
        client.write_text(json.dumps({"schema": AB.CLIENT_SCHEMA, "started_at": "2026-08-12T00:00:00Z", "ended_at": "2026-08-12T00:00:01Z", "http_status": 200, "wall_ms": wall_ms, "ttft_ms": wall_ms / 10, "itl_ms": [1000 / generation_tps] * 7}), encoding="utf-8")
        return response, client

    def test_plan_and_schedule_are_model_general_and_balanced(self) -> None:
        AB.validate_plan(copy.deepcopy(self.plan))
        schedule = AB.make_schedule(self.plan)
        self.assertEqual(len(schedule["entries"]), 6)
        for pair in range(1, 4):
            self.assertEqual({entry["condition"] for entry in schedule["entries"] if entry["pair_id"] == pair}, {"off", "on"})

    def test_v1_normalized_documents_and_canonical_identity_are_unchanged(self) -> None:
        example = Path(__file__).parents[1] / "scripts" / "halofpx-strix-ab-plan.example.json"
        plan = AB.load_plan(example)
        normalized = lambda value: (json.dumps(value, indent=2, sort_keys=True) + "\n").encode()
        self.assertEqual(hashlib.sha256(normalized(plan)).hexdigest(),
                         "267e265d19a78ba2050936818bc35481da82486a022619dd3e5aada5ba4350f0")
        self.assertEqual(hashlib.sha256(normalized(AB.make_schedule(plan))).hexdigest(),
                         "eb48d0691c01936b11b79b4b07e3c85f19ec90c248de50b16c09bdc7907480b2")
        self.assertEqual(hashlib.sha256(normalized(AB.commands_document(plan))).hexdigest(),
                         "285bae02c3b45ba7c64aaa981541669bd932f0787229bfa611f67cf9d9478ebf")
        self.assertEqual(AB.plan_digest(plan),
                         "f6f1d7fc4e1aaec6b0dffddadb5eb539aaba1ad881b8f95b181394707bb76aeb")

    def test_v2_runtime_batch_uses_identical_artifacts_and_one_generated_flag(self) -> None:
        self.plan = self.make_v2()
        AB.validate_plan(copy.deepcopy(self.plan))
        commands = AB.commands_document(self.plan)["conditions"]
        self.assertEqual(commands["off"]["worker"], commands["on"]["worker"])
        self.assertEqual(commands["off"]["coordinator"][-2:], ["--batch-size", "512"])
        self.assertEqual(commands["on"]["coordinator"][-2:], ["--batch-size", "2048"])
        self.assertEqual(commands["off"]["coordinator"][:-2], commands["on"]["coordinator"][:-2])
        for condition in ("off", "on"):
            self.assertEqual(commands[condition]["coordinator"].count("--batch-size"), 1)

        run = self.initialize()
        self.import_preflights(run)
        schedule = json.loads((run / "schedule.json").read_text(encoding="utf-8"))
        for entry in schedule["entries"]:
            response, client = self.raw_success(
                110.0 if entry["condition"] == "on" else 100.0,
                22.0 if entry["condition"] == "on" else 20.0,
                900.0 if entry["condition"] == "on" else 1000.0,
            )
            AB.record_sample(
                run, entry["pair_id"], entry["condition"], entry["order_index"],
                response, client, "success", None, [])
        report = AB.analyze_run(run)
        self.assertEqual(report["schema"], AB.ANALYSIS_SCHEMA_V2)
        self.assertEqual(report["comparison"]["kind"], "runtime_n_batch")
        self.assertEqual(report["comparison"]["batch_by_condition"], {"off": 512, "on": 2048})
        self.assertEqual(report["comparison"]["ubatch"], 512)
        self.assertEqual(
            report["comparison"]["condition_commands_sha256"],
            {name: AB.digest_bytes(AB.canonical_bytes(AB.condition_commands(self.plan, name)))
             for name in ("off", "on")},
        )

    def test_v2_runtime_batch_rejects_every_untyped_or_unmatched_control(self) -> None:
        cases = []
        broken = self.make_v2()
        broken["runtime"]["batch_by_condition"]["on"] = 512
        cases.append(("wrong-batch", broken))
        broken = self.make_v2()
        broken["runtime"]["batch_by_condition"]["on"] = True
        cases.append(("boolean-batch", broken))
        broken = self.make_v2()
        broken["runtime"]["batch_by_condition"]["on"] = "2048"
        cases.append(("string-batch", broken))
        broken = self.make_v2()
        broken["runtime"]["batch_by_condition"] = {"off": 2048, "on": 512}
        cases.append(("swapped-batches", broken))
        broken = self.make_v2()
        broken["runtime"]["batch"] = 512
        cases.append(("obsolete-v1-batch", broken))
        broken = self.make_v2()
        broken["runtime"]["ubatch"] = 256
        cases.append(("wrong-ubatch", broken))
        broken = self.make_v2()
        broken["source"]["on_commit"] = "1" * 40
        broken["conditions"]["on"]["source_commit"] = "1" * 40
        cases.append(("different-commit", broken))
        broken = self.make_v2()
        broken["conditions"]["on"]["coordinator_binary"]["path"] = str(self.files["on-server"])
        cases.append(("different-path", broken))
        broken = self.make_v2()
        broken["conditions"]["on"]["worker_binary"]["sha256"] = "f" * 64
        cases.append(("different-hash", broken))
        for name, extra in (
                ("long-batch", ["--batch-size=2048"]),
                ("short-batch", ["-b", "2048"]),
                ("short-ubatch", ["-ub", "512"]),
                ("underscore-batch", ["--batch_size", "4096"]),
                ("underscore-batch-equals", ["--batch_size=4096"]),
                ("underscore-ubatch", ["--ubatch_size", "4096"]),
                ("underscore-ubatch-equals", ["--ubatch_size=4096"])):
            broken = self.make_v2()
            broken["runtime"]["common_coordinator_args"].extend(extra)
            cases.append((name, broken))
        broken = self.make_v2()
        broken["runtime"]["common_environment"]["LLAMA_ARG_BATCH"] = "2048"
        cases.append(("batch-env", broken))
        broken = self.make_v2()
        broken["runtime"]["common_environment"]["LLAMA_ARG_UBATCH"] = "512"
        cases.append(("ubatch-env", broken))
        broken = self.make_v2()
        args = broken["runtime"]["common_coordinator_args"]
        index = args.index("--ubatch-size")
        del args[index:index + 2]
        args.append("--ubatch-size=512")
        cases.append(("noncanonical-ubatch", broken))
        broken = self.make_v2()
        broken["comparison"]["control"] = "on"
        cases.append(("swapped-control", broken))
        broken = self.make_v2()
        broken["comparison"]["candidate"] = "off"
        cases.append(("swapped-candidate", broken))
        broken = self.make_v2()
        broken["comparison"]["kind"] = "arbitrary"
        cases.append(("unknown-kind", broken))
        broken = self.make_v2()
        broken["comparison"]["extra"] = True
        cases.append(("unknown-comparison-field", broken))
        for name, plan in cases:
            with self.subTest(name=name), self.assertRaises(AB.PlanError):
                AB.validate_plan(plan)

    def test_v2_feature_build_keeps_distinct_binary_rule(self) -> None:
        plan = self.make_v2("feature_build")
        AB.validate_plan(copy.deepcopy(plan))
        for key in ("coordinator_binary", "worker_binary"):
            plan["conditions"]["on"][key]["sha256"] = plan["conditions"]["off"][key]["sha256"]
        with self.assertRaisesRegex(AB.PlanError, "distinct OFF/ON binary"):
            AB.validate_plan(plan)

    def test_v1_same_binary_hashes_with_distinct_paths_remains_accepted_by_core(self) -> None:
        plan = copy.deepcopy(self.plan)
        plan["source"]["on_commit"] = plan["source"]["off_commit"]
        plan["conditions"]["on"]["source_commit"] = plan["source"]["off_commit"]
        for key in ("coordinator_binary", "worker_binary"):
            plan["conditions"]["on"][key]["sha256"] = plan["conditions"]["off"][key]["sha256"]
        AB.validate_plan(plan)

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

    def test_preflight_retains_exact_request_and_authority_bytes(self) -> None:
        self.files["nimo1"].write_bytes(b"authority\r\nexact\x00bytes")
        self.plan["topology"]["coordinator"]["authority_receipt"]["sha256"] = sha(self.files["nimo1"])
        run = self.initialize()
        self.import_preflights(run)
        expected_request = self.files["request"].read_bytes()
        expected_authority = self.files["nimo1"].read_bytes()
        self.assertEqual((run / "inputs" / "request.raw").read_bytes(), expected_request)
        self.assertEqual((run / "inputs" / "authority-coordinator.raw").read_bytes(), expected_authority)
        for source in (self.files["request"], self.files["nimo1"], self.files["nimo2"]):
            source.unlink()
        self.assertEqual(set(AB.validate_preflights(run, self.plan)), {"coordinator", "worker"})

    def test_request_semantics_fail_closed(self) -> None:
        base = json.loads(self.files["request"].read_text(encoding="utf-8"))
        cases = {
            "cache-on": {**base, "cache_prompt": True},
            "cache-zero": {**base, "cache_prompt": 0},
            "not-streamed": {**base, "stream": False},
            "wrong-count": {**base, "n_predict": 7},
            "bool-count": {**base, "n_predict": True},
            "random-seed": {**base, "seed": 0xffffffff},
            "warm-sampling": {**base, "temperature": 0.1},
            "early-eog": {**base, "ignore_eos": False},
            "unknown": {**base, "extra": 1},
        }
        for name, request in cases.items():
            with self.subTest(name=name), self.assertRaises(AB.PlanError):
                AB.validate_completion_request(json.dumps(request).encode(), self.plan)
        duplicate = b'{"prompt":"x","n_predict":8,"stream":true,"cache_prompt":false,"ignore_eos":true,"seed":1,"temperature":0,"cache_prompt":false}'
        with self.assertRaises(AB.PlanError):
            AB.validate_completion_request(duplicate, self.plan)

    def test_request_allows_multiline_exact_prompt_and_rejects_duplicate_evidence_json(self) -> None:
        request = json.loads(self.files["request"].read_text(encoding="utf-8"))
        request["prompt"] = "system\nuser\tcontent"
        self.assertEqual(
            AB.validate_completion_request(json.dumps(request).encode(), self.plan)["prompt"],
            request["prompt"],
        )
        duplicate_path = self.root / "duplicate.json"
        duplicate_path.write_text('{"cache_n":511,"cache_n":0}', encoding="utf-8")
        with self.assertRaises(AB.PlanError):
            AB.read_json(duplicate_path)

    def test_import_rejects_tampered_retained_content(self) -> None:
        run = self.initialize()
        receipt = AB.collect_preflight(self.plan, "worker", observed_hostname="nimo-2")
        receipt["artifacts"]["authority_receipt"]["content_base64"] = "YQ=="
        path = self.root / "tampered-worker.json"
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
        self.assertTrue(report["evidence_core_complete"])
        self.assertFalse(report["execution_qualified"])
        self.assertFalse(report["measurement_ready"])
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
        self.assertFalse(report["evidence_core_complete"])
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

    def test_response_rejects_missing_noninteger_or_nonzero_cache_count(self) -> None:
        response, _ = self.raw_success(100.0, 20.0, 1000.0)
        original = json.loads(response.read_text(encoding="utf-8"))
        cases = {
            "missing": None,
            "boolean": True,
            "float": 0.0,
            "nonzero": 511,
        }
        for name, cache_n in cases.items():
            with self.subTest(name=name):
                candidate = copy.deepcopy(original)
                if cache_n is None:
                    del candidate["timings"]["cache_n"]
                else:
                    candidate["timings"]["cache_n"] = cache_n
                response.write_text(json.dumps(candidate), encoding="utf-8")
                with self.assertRaises(AB.PlanError):
                    AB.parse_response(response, self.plan)

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
