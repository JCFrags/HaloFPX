from __future__ import annotations

import copy
import importlib.util
import json
import tempfile
import unittest
from pathlib import Path


ROOT = Path(__file__).parents[1]


def load_module(name: str, path: Path):
    spec = importlib.util.spec_from_file_location(name, path)
    assert spec and spec.loader
    module = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(module)
    return module


AB = load_module("halofpx_strix_ab_sampling_test_core", ROOT / "scripts" / "halofpx_strix_ab.py")
SYNC = load_module(
    "halofpx_strix_ab_sampling_test_sidecar",
    ROOT / "scripts" / "halofpx_strix_ab_sampling_sync.py")
BASE = load_module("halofpx_strix_ab_sampling_base_tests", ROOT / "tests" / "test_halofpx_strix_ab.py")
FAKE = load_module(
    "halofpx_strix_ab_sampling_fake",
    ROOT / "tests" / "halofpx_strix_ab_sampling_sync_fake.py")


class SamplingSyncObservabilityTest(unittest.TestCase):
    def setUp(self) -> None:
        self.fixture = BASE.StrixABTest("test_plan_and_schedule_are_model_general_and_balanced")
        self.fixture.setUp()
        self.root = self.fixture.root
        self.plan = self.fixture.plan
        self.fake = FAKE.FakeSamplingOutputSyncAdapter()

    def tearDown(self) -> None:
        self.fixture.tearDown()

    def enable_contract(self) -> tuple[Path, dict]:
        self.plan["issues"].append(28)
        self.plan["source"]["on_commit"] = self.plan["source"]["off_commit"]
        self.plan["conditions"]["on"]["source_commit"] = self.plan["source"]["off_commit"]
        self.plan["runtime"]["common_coordinator_args"].extend([
            "--metrics", "--parallel", "1", "--no-cont-batching", "--no-warmup"])
        run = self.fixture.initialize()
        side_plan = {
            "schema": SYNC.PLAN_SCHEMA,
            "lane": SYNC.CONTRACT,
            "enabled": True,
            "issue": 28,
            "core_plan_sha256": AB.plan_digest(self.plan),
            "endpoint": "/metrics",
            "completion_endpoint": "/completion",
            "metrics": SYNC.METRICS,
            "conditions": {"off": {"coalescing": False}, "on": {"coalescing": True}},
        }
        side_path = self.root / "sampling-plan.json"
        AB.write_json(side_path, side_plan)
        SYNC.freeze_observability_plan(run, side_path)
        return run, side_plan

    def record_all(self, run: Path, *, delta_override=None) -> None:
        schedule = AB.read_json(run / "schedule.json")
        for entry in schedule["entries"]:
            on = entry["condition"] == "on"
            response, client = self.fixture.raw_success(
                110.0 if on else 100.0, 22.0 if on else 20.0, 900.0 if on else 1000.0)
            AB.record_sample(
                run, entry["pair_id"], entry["condition"], entry["order_index"],
                response, client, "success", None, [])
            override = delta_override(entry) if delta_override else None
            evidence = self.fake.emit(
                self.root / f"fake-{entry['pair_id']}-{entry['condition']}",
                entry["condition"], self.plan["request"]["sha256"],
                self.plan["request"]["output_tokens"],
                response_sha256=AB.digest_file(response), client_sha256=AB.digest_file(client),
                delta_overrides=override)
            SYNC.record_observation(
                run, entry["pair_id"], entry["condition"], entry["order_index"], *evidence)

    def test_absent_contract_preserves_v1_plan_sample_and_analysis_shape(self) -> None:
        original = copy.deepcopy(self.plan)
        run = self.fixture.initialize()
        self.fixture.import_preflights(run)
        schedule = AB.read_json(run / "schedule.json")
        for entry in schedule["entries"]:
            response, client = self.fixture.raw_success(100.0, 20.0, 1000.0)
            AB.record_sample(
                run, entry["pair_id"], entry["condition"], entry["order_index"],
                response, client, "success", None, [])
        self.assertIsNone(SYNC.validate_frozen_run(run))
        report = AB.analyze_run(run)
        self.assertEqual(AB.read_json(run / "plan.json"), original)
        self.assertEqual(report["schema"], AB.ANALYSIS_SCHEMA)
        self.assertNotIn("observability", report)
        sample = AB.read_json(next((run / "raw").glob("*/sample.json")))
        self.assertEqual(set(sample), {
            "schema", "experiment_id", "plan_sha256", "pair_id", "order_index",
            "condition", "status", "failure_code", "identity", "client", "result", "raw"})

        counterfeit = run / SYNC.ANALYSIS_FILENAME
        AB.write_json(counterfeit, {"enabled": True, "evidence_complete": True})
        with self.assertRaisesRegex(SYNC.PlanError, "analysis exists without"):
            SYNC.validate_frozen_run(run)
        with self.assertRaisesRegex(AB.PlanError, "analysis exists without"):
            AB.analyze_run(run)
        counterfeit.unlink()

        reserved_plan = run / SYNC.PLAN_FILENAME
        reserved_plan.mkdir()
        with self.assertRaisesRegex(SYNC.PlanError, "plan is not a regular file"):
            SYNC.validate_frozen_run(run)
        with self.assertRaisesRegex(AB.PlanError, "plan is not a regular file"):
            AB.analyze_run(run)

    def test_checked_in_default_off_sidecar_binds_unchanged_v1_example(self) -> None:
        core_plan = AB.load_plan(ROOT / "scripts" / "halofpx-strix-ab-plan.example.json")
        side_plan = SYNC.load_observability_plan(
            ROOT / "scripts" / "halofpx-strix-ab-sampling-output-sync-prometheus.example.json",
            core_plan)
        self.assertFalse(side_plan["enabled"])
        self.assertEqual(
            AB.plan_digest(core_plan),
            "f6f1d7fc4e1aaec6b0dffddadb5eb539aaba1ad881b8f95b181394707bb76aeb")

    def test_explicit_disabled_contract_needs_no_flags_and_rejects_evidence(self) -> None:
        run = self.fixture.initialize()
        side_plan = {
            "schema": SYNC.PLAN_SCHEMA,
            "lane": SYNC.CONTRACT,
            "enabled": False,
            "issue": 28,
            "core_plan_sha256": AB.plan_digest(self.plan),
            "endpoint": "/metrics",
            "completion_endpoint": "/completion",
            "metrics": SYNC.METRICS,
            "conditions": {"off": {"coalescing": False}, "on": {"coalescing": True}},
        }
        side_path = self.root / "disabled.json"
        AB.write_json(side_path, side_plan)

        counterfeit = run / SYNC.ANALYSIS_FILENAME
        AB.write_json(counterfeit, {"enabled": True, "evidence_complete": True})
        with self.assertRaisesRegex(SYNC.PlanError, "analysis exists before"):
            SYNC.freeze_observability_plan(run, side_path)
        counterfeit.unlink()

        reserved_plan = run / SYNC.PLAN_FILENAME
        reserved_plan.mkdir()
        with self.assertRaisesRegex(SYNC.PlanError, "plan already exists"):
            SYNC.freeze_observability_plan(run, side_path)
        reserved_plan.rmdir()

        SYNC.freeze_observability_plan(run, side_path)
        report = SYNC.validate_frozen_run(run)
        self.assertFalse(report["enabled"])
        evidence_dir = run / "raw" / "orphan" / SYNC.EVIDENCE_DIRECTORY
        evidence_dir.mkdir(parents=True)
        with self.assertRaisesRegex(SYNC.PlanError, "disabled"):
            SYNC.validate_frozen_run(run)

    def test_enabled_plan_requires_issue_and_exact_single_request_flags(self) -> None:
        plan = copy.deepcopy(self.plan)
        side = {
            "schema": SYNC.PLAN_SCHEMA,
            "lane": SYNC.CONTRACT,
            "enabled": True,
            "issue": 28,
            "core_plan_sha256": AB.plan_digest(plan),
            "endpoint": "/metrics",
            "completion_endpoint": "/completion",
            "metrics": SYNC.METRICS,
            "conditions": {"off": {"coalescing": False}, "on": {"coalescing": True}},
        }
        with self.assertRaisesRegex(SYNC.PlanError, "issue 28"):
            SYNC.validate_observability_plan(side, plan)
        plan["issues"].append(28)
        plan["source"]["on_commit"] = plan["source"]["off_commit"]
        plan["conditions"]["on"]["source_commit"] = plan["source"]["off_commit"]
        side["core_plan_sha256"] = AB.plan_digest(plan)
        required = ["--metrics", "--parallel", "1", "--no-cont-batching", "--no-warmup"]
        plan["runtime"]["common_coordinator_args"].extend(required)
        side["core_plan_sha256"] = AB.plan_digest(plan)
        SYNC.validate_observability_plan(side, plan)
        for item in ("--metrics", "--parallel", "--no-cont-batching", "--no-warmup"):
            broken = copy.deepcopy(plan)
            index = broken["runtime"]["common_coordinator_args"].index(item)
            del broken["runtime"]["common_coordinator_args"][index]
            if item == "--parallel":
                del broken["runtime"]["common_coordinator_args"][index]
            candidate = dict(side, core_plan_sha256=AB.plan_digest(broken))
            with self.subTest(missing=item), self.assertRaises(SYNC.PlanError):
                SYNC.validate_observability_plan(candidate, broken)
        broken = copy.deepcopy(plan)
        broken["runtime"]["common_coordinator_args"].extend(["--parallel", "2"])
        with self.assertRaises(SYNC.PlanError):
            SYNC.validate_observability_plan(
                dict(side, core_plan_sha256=AB.plan_digest(broken)), broken)
        for name, mutate in {
            "equals": lambda value: value["runtime"]["common_coordinator_args"].extend(["--metrics=true"]),
            "underscore": lambda value: value["runtime"]["common_coordinator_args"].extend(["--no_cont_batching"]),
            "opposite": lambda value: value["runtime"]["common_coordinator_args"].extend(["--cont-batching"]),
            "short-parallel": lambda value: value["runtime"]["common_coordinator_args"].extend(["-np", "1"]),
            "short-no-cont": lambda value: value["runtime"]["common_coordinator_args"].extend(["-nocb"]),
            "duplicate-port": lambda value: value["runtime"]["common_coordinator_args"].extend(
                ["--port", "18081"]),
            "environment": lambda value: value["runtime"]["common_environment"].update(
                LLAMA_ARG_N_PARALLEL="2"),
        }.items():
            broken = copy.deepcopy(plan)
            mutate(broken)
            candidate = dict(side, core_plan_sha256=AB.plan_digest(broken))
            with self.subTest(noncanonical=name), self.assertRaises(SYNC.PlanError):
                SYNC.validate_observability_plan(candidate, broken)

    def test_enabled_lane_accepts_v2_feature_build_and_rejects_runtime_batch(self) -> None:
        required = ["--metrics", "--parallel", "1", "--no-cont-batching", "--no-warmup"]
        for kind in ("feature_build", "runtime_n_batch"):
            plan = self.fixture.make_v2(kind)
            plan["issues"].append(28)
            plan["source"]["on_commit"] = plan["source"]["off_commit"]
            plan["conditions"]["on"]["source_commit"] = plan["source"]["off_commit"]
            plan["runtime"]["common_coordinator_args"].extend(required)
            side = {
                "schema": SYNC.PLAN_SCHEMA,
                "lane": SYNC.CONTRACT,
                "enabled": True,
                "issue": 28,
                "core_plan_sha256": AB.plan_digest(plan),
                "endpoint": "/metrics",
                "completion_endpoint": "/completion",
                "metrics": SYNC.METRICS,
                "conditions": {"off": {"coalescing": False}, "on": {"coalescing": True}},
            }
            if kind == "feature_build":
                SYNC.validate_observability_plan(side, plan)
            else:
                with self.assertRaisesRegex(SYNC.PlanError, "feature-build comparison"):
                    SYNC.validate_observability_plan(side, plan)

        plan = copy.deepcopy(self.plan)
        plan["issues"].append(28)
        plan["runtime"]["common_coordinator_args"].extend(required)
        side["core_plan_sha256"] = AB.plan_digest(plan)
        with self.assertRaisesRegex(SYNC.PlanError, "identical OFF and ON source commits"):
            SYNC.validate_observability_plan(side, plan)

    def test_prometheus_parser_retains_exact_uint64_above_2_pow_53(self) -> None:
        values = {
            name: (1 << 53) + index + 17
            for index, name in enumerate(SYNC.METRICS)
        }
        values["output_transfers"] = SYNC.UINT64_MAX
        parsed = SYNC.parse_prometheus_snapshot(self.fake.prometheus(values), "fixture")
        self.assertEqual(parsed, values)
        self.assertIsInstance(parsed["output_transfers"], int)

    def test_prometheus_parser_fails_on_missing_duplicate_labels_format_and_overflow(self) -> None:
        values = {name: 10 + index for index, name in enumerate(SYNC.METRICS)}
        valid = self.fake.prometheus(values).decode("utf-8")
        target = SYNC.METRICS["reused_barriers"]
        cases = {
            "missing": "\n".join(line for line in valid.splitlines() if not line.startswith(target + " ")),
            "duplicate": valid + f"{target} 12\n",
            "labels": valid.replace(f"{target} {values['reused_barriers']}", f'{target}{{slot="0"}} 12'),
            "float": valid.replace(f"{target} {values['reused_barriers']}", f"{target} 12.0"),
            "overflow": valid.replace(
                f"{target} {values['reused_barriers']}", f"{target} {SYNC.UINT64_MAX + 1}"),
            "stale": valid + f"{SYNC.STALE_METRIC} 1\n",
            "stale-type": valid + f"# TYPE {SYNC.STALE_METRIC} counter\n",
            "missing-type": valid.replace(f"# TYPE {target} counter\n", ""),
            "duplicate-type": valid + f"# TYPE {target} counter\n",
            "wrong-type": valid.replace(f"# TYPE {target} counter", f"# TYPE {target} gauge"),
        }
        for name, text in cases.items():
            with self.subTest(name=name), self.assertRaises(SYNC.PlanError):
                SYNC.parse_prometheus_snapshot(text.encode("utf-8"), name)

    def test_delta_and_condition_gates_fail_closed(self) -> None:
        before = {name: 20 for name in SYNC.METRICS}
        after = dict(before)
        after["completed_barriers"] = 19
        with self.assertRaisesRegex(SYNC.PlanError, "decreased"):
            SYNC.counter_delta(before, after)
        near_max = {name: SYNC.UINT64_MAX - 5 for name in SYNC.METRICS}
        at_max = {name: SYNC.UINT64_MAX for name in SYNC.METRICS}
        self.assertEqual(SYNC.counter_delta(near_max, at_max), {name: 5 for name in SYNC.METRICS})
        valid = {
            "output_epochs": 8, "completed_barriers": 48, "reused_barriers": 0,
            "graph_submissions": 8, "output_transfers": 8,
        }
        SYNC.validate_condition_delta("off", valid)
        cases = [
            ("off", dict(valid, reused_barriers=1)),
            ("on", dict(valid, completed_barriers=8, reused_barriers=0)),
            ("on", dict(valid, completed_barriers=8, reused_barriers=40, graph_submissions=0)),
            ("on", dict(valid, completed_barriers=8, reused_barriers=40, output_transfers=0)),
        ]
        for condition, delta in cases:
            with self.assertRaises(SYNC.PlanError):
                SYNC.validate_condition_delta(condition, delta)

    def test_capture_requires_exactly_one_request_strict_order_and_one_identity(self) -> None:
        run, _ = self.enable_contract()
        evidence = self.fake.emit(
            self.root / "identity", "off", self.plan["request"]["sha256"], 8)
        capture = AB.read_json(evidence[2])
        for name, mutate in {
            "count": lambda value: value["request"].update(request_count=2),
            "boolean-count": lambda value: value["request"].update(request_count=True),
            "pid": lambda value: value["after"]["identity"].update(pid=9999),
            "invocation": lambda value: value["request"]["identity"].update(invocation_id="f" * 32),
            "process-start": lambda value: value["before"]["identity"].update(process_start_ticks=99),
            "metrics-process-start": lambda value: value["after"]["identity"].update(
                metrics_process_start_time_unix=99),
            "order": lambda value: value["after"].update(captured_monotonic_ns=1),
            "hash": lambda value: value["before"].update(metrics_sha256="0" * 64),
            "request-hash": lambda value: value["request"].update(request_sha256="0" * 64),
            "response-hash": lambda value: value["request"].update(response_sha256="0" * 64),
            "client-hash": lambda value: value["request"].update(client_sha256="0" * 64),
            "endpoint": lambda value: value["endpoint"].update(port=18081),
        }.items():
            broken = copy.deepcopy(capture)
            mutate(broken)
            path = self.root / f"capture-{name}.json"
            AB.write_json(path, broken)
            with self.subTest(name=name), self.assertRaises(SYNC.PlanError):
                SYNC.build_sample_summary(
                    self.plan, AB.read_json(run / SYNC.PLAN_FILENAME), 1, "off", 0,
                    evidence[0], evidence[1], path, "a" * 64, "b" * 64)

    def test_valid_full_pair_analysis_is_exact_and_core_hook_accepts_it(self) -> None:
        run, _ = self.enable_contract()
        self.fixture.import_preflights(run)
        self.record_all(run)
        report = SYNC.validate_frozen_run(run)
        self.assertTrue(report["evidence_complete"])
        self.assertEqual(len(report["pairs"]), 3)
        self.assertEqual(
            report["pairs"][0]["off_single_process_window_delta"]["reused_barriers"], "0")
        self.assertGreater(
            int(report["pairs"][0]["on_single_process_window_delta"]["reused_barriers"]), 0)
        summary = AB.read_json(next((run / "raw").glob("*/sampling-output-sync/summary.json")))
        exact = summary["counters"]["before"]["output_epochs"]
        self.assertIsInstance(exact, str)
        self.assertGreater(int(exact), 1 << 53)
        self.assertEqual(summary["capture"]["endpoint"], {
            "scheme": "http", "host": "nimo-1", "port": 18080,
            "metrics_path": "/metrics", "completion_path": "/completion"})
        core_report = AB.analyze_run(run)
        self.assertTrue(core_report["evidence_core_complete"])
        sums = (run / "SHA256SUMS").read_text(encoding="utf-8")
        self.assertIn(SYNC.ANALYSIS_FILENAME, sums)
        self.assertIn("sampling-output-sync/before.prom", sums)

    def test_enabled_analysis_rejects_orphan_and_unreferenced_sidecar_artifacts(self) -> None:
        run, _ = self.enable_contract()
        self.fixture.import_preflights(run)
        self.record_all(run)

        orphan = run / "raw" / "orphan" / SYNC.EVIDENCE_DIRECTORY
        orphan.mkdir(parents=True)
        (orphan / "counterfeit.prom").write_text("counterfeit\n", encoding="utf-8")
        with self.assertRaisesRegex(SYNC.PlanError, "directories differ from the frozen schedule"):
            SYNC.validate_frozen_run(run)
        with self.assertRaisesRegex(AB.PlanError, "directories differ from the frozen schedule"):
            AB.analyze_run(run)
        (orphan / "counterfeit.prom").unlink()
        orphan.rmdir()
        orphan.parent.rmdir()

        retained = next((run / "raw").glob(f"*/{SYNC.EVIDENCE_DIRECTORY}"))
        extra = retained / "unreferenced.txt"
        extra.write_text("unreferenced\n", encoding="utf-8")
        with self.assertRaisesRegex(SYNC.PlanError, "must contain exactly"):
            SYNC.validate_frozen_run(run)
        extra.unlink()

        reserved_analysis = run / SYNC.ANALYSIS_FILENAME
        reserved_analysis.mkdir()
        with self.assertRaisesRegex(SYNC.PlanError, "analysis is not a regular file"):
            SYNC.validate_frozen_run(run)
        with self.assertRaisesRegex(AB.PlanError, "analysis is not a regular file"):
            AB.analyze_run(run)

    def test_pr51_evidence_validator_handoff_is_explicit_and_fail_closed(self) -> None:
        self.assertEqual(SYNC.EVIDENCE_VALIDATOR_ROOT_FILES, (
            "sampling-output-sync-plan.json",
            "sampling-output-sync-analysis.json",
        ))
        self.assertEqual(SYNC.EVIDENCE_VALIDATOR_SAMPLE_FILES, (
            "before.prom", "after.prom", "capture.json", "summary.json",
        ))
        contract = (ROOT / "docs" / "halofpx" / "strix-ab-harness.md").read_text(
            encoding="utf-8")
        for required in (
            "A complete PR #51 adapter evidence verifier **MUST** detect",
            "`sampling_output_sync_prometheus_v1`",
            "authoritative `validate_frozen_run` reparse",
            "does not implement this versioned profile **MUST** refuse",
            "`evidence_complete` alone establishes",
        ):
            self.assertIn(required, contract)

    def test_pair_analysis_rejects_on_completed_not_lower(self) -> None:
        run, _ = self.enable_contract()
        self.record_all(run, delta_override=lambda entry: (
            {"completed_barriers": 48} if entry["condition"] == "on" else None))
        with self.assertRaisesRegex(SYNC.PlanError, "not lower"):
            SYNC.validate_frozen_run(run)

    def test_pair_gate_rejects_epoch_graph_or_transfer_mismatch(self) -> None:
        off = {
            "output_epochs": 9, "completed_barriers": 48, "reused_barriers": 0,
            "graph_submissions": 12, "output_transfers": 8,
        }
        on = {
            "output_epochs": 9, "completed_barriers": 8, "reused_barriers": 40,
            "graph_submissions": 12, "output_transfers": 8,
        }
        SYNC.validate_pair_deltas(off, on)
        for name in ("output_epochs", "graph_submissions", "output_transfers"):
            broken = dict(on)
            broken[name] += 1
            with self.subTest(name=name), self.assertRaisesRegex(SYNC.PlanError, name):
                SYNC.validate_pair_deltas(off, broken)
        for reused in (1, 50):
            broken = dict(on, reused_barriers=reused)
            with self.subTest(reused=reused), self.assertRaisesRegex(
                    SYNC.PlanError, "total synchronization-decision"):
                SYNC.validate_pair_deltas(off, broken)

    def test_enabled_lane_retains_core_output_token_gate(self) -> None:
        run, _ = self.enable_contract()
        first = AB.read_json(run / "schedule.json")["entries"][0]
        response, client = self.fixture.raw_success(100.0, 20.0, 1000.0)
        value = AB.read_json(response)
        value["timings"]["predicted_n"] = 7
        AB.write_json(response, value)
        with self.assertRaisesRegex(AB.PlanError, "token counts"):
            AB.record_sample(
                run, first["pair_id"], first["condition"], first["order_index"],
                response, client, "success", None, [])

if __name__ == "__main__":
    unittest.main()
