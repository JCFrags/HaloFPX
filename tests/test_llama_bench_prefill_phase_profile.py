#!/usr/bin/env python3
"""Focused CPU contract tests for llama-bench's opt-in prefill profile."""

from __future__ import annotations

import json
import os
import subprocess
import unittest
from pathlib import Path


PROFILE_FIELD = "prefill_phase_profile_samples"
PROFILE_KEYS = {
    "graph_reset_wall_ns",
    "graph_build_wall_ns",
    "scheduler_dispatch_wall_ns",
    "scheduler_synchronize_wall_ns",
    "successful_ubatch_count",
    "graph_build_count",
    "graph_reuse_count",
    "rpc_stats_available",
}


class LlamaBenchPrefillPhaseProfileTest(unittest.TestCase):
    @classmethod
    def setUpClass(cls) -> None:
        bench_env = os.environ.get("HALOFPX_LLAMA_BENCH_BIN")
        model_env = os.environ.get("HALOFPX_CANARY_MODEL")
        if not bench_env or not model_env:
            raise unittest.SkipTest(
                "set HALOFPX_LLAMA_BENCH_BIN and HALOFPX_CANARY_MODEL to run the integration fixture"
            )

        cls.bench = Path(bench_env).resolve()
        cls.model = Path(model_env).resolve()
        if not cls.bench.is_file():
            raise AssertionError(f"llama-bench executable does not exist: {cls.bench}")
        if not cls.model.is_file():
            raise AssertionError(f"tiny GGUF fixture does not exist: {cls.model}")

    def run_bench(
        self, output_format: str, repetitions: int, profile: bool, n_gen: int = 0
    ) -> str:
        command = [
            str(self.bench),
            "-m",
            str(self.model),
        ]
        if n_gen > 0:
            command.extend(["-pg", f"8,{n_gen}"])
        else:
            command.extend(["-p", "8", "-n", "0"])
        command.extend([
            "-b",
            "8",
            "-ub",
            "4",
            "-t",
            "1",
            "-ngl",
            "0",
            "-fa",
            "off",
            "-r",
            str(repetitions),
            "--no-warmup",
            "-o",
            output_format,
        ])
        if profile:
            command.append("--prefill-phase-profile")

        completed = subprocess.run(
            command,
            check=False,
            capture_output=True,
            encoding="utf-8",
            errors="replace",
            timeout=120,
        )
        self.assertEqual(
            completed.returncode,
            0,
            msg=f"command failed: {' '.join(command)}\nstdout:\n{completed.stdout}\nstderr:\n{completed.stderr}",
        )
        return completed.stdout

    def assert_profile_row(self, row: dict[str, object], repetitions: int) -> None:
        self.assertEqual(len(row["samples_ns"]), repetitions)
        samples = row[PROFILE_FIELD]
        self.assertIsInstance(samples, list)
        self.assertEqual(len(samples), repetitions)

        for sample in samples:
            self.assertIsInstance(sample, dict)
            self.assertEqual(set(sample), PROFILE_KEYS)
            self.assertIs(sample["rpc_stats_available"], False)

            for key in PROFILE_KEYS - {"rpc_stats_available"}:
                self.assertIs(type(sample[key]), int)
                self.assertGreaterEqual(sample[key], 0)

            # Eight prompt tokens at ubatch four must dispatch exactly two
            # successful ubatches. Each is either a graph build or a reuse.
            self.assertEqual(sample["successful_ubatch_count"], 2)
            self.assertEqual(sample["graph_build_count"] + sample["graph_reuse_count"], 2)

    def test_feature_off_json_schema_is_unchanged(self) -> None:
        rows = json.loads(self.run_bench("json", repetitions=1, profile=False))
        self.assertEqual(len(rows), 1)
        self.assertNotIn(PROFILE_FIELD, rows[0])

    def test_json_profile_has_one_sample_per_repetition(self) -> None:
        rows = json.loads(self.run_bench("json", repetitions=3, profile=True))
        self.assertEqual(len(rows), 1)
        self.assert_profile_row(rows[0], repetitions=3)

    def test_jsonl_profile_has_one_sample_per_repetition(self) -> None:
        lines = [line for line in self.run_bench("jsonl", repetitions=2, profile=True).splitlines() if line]
        self.assertEqual(len(lines), 1)
        self.assert_profile_row(json.loads(lines[0]), repetitions=2)

    def test_combined_prompt_generation_row_keeps_profile_cardinality(self) -> None:
        rows = json.loads(
            self.run_bench("json", repetitions=2, profile=True, n_gen=1)
        )
        combined = [
            row for row in rows if row["n_prompt"] == 8 and row["n_gen"] == 1
        ]
        self.assertEqual(len(combined), 1)
        self.assert_profile_row(combined[0], repetitions=2)

    def test_non_json_formats_do_not_gain_profile_columns(self) -> None:
        for output_format in ("csv", "md", "sql"):
            with self.subTest(output_format=output_format):
                output = self.run_bench(output_format, repetitions=1, profile=True)
                self.assertNotIn(PROFILE_FIELD, output)


class LlamaBenchPrefillPhaseTimingSourceTest(unittest.TestCase):
    def test_diagnostic_readback_is_outside_workload_timing(self) -> None:
        source = (
            Path(__file__).resolve().parents[1]
            / "tools"
            / "llama-bench"
            / "llama-bench.cpp"
        ).read_text(encoding="utf-8")

        prompt_boundary = source.index("t_prompt_end = get_time_ns();")
        profile_readback = source.index(
            "prefill_phase_profile_samples.push_back(llama_perf_prefill_phase(ctx))"
        )
        generation_boundary = source.index("t_generation_start = get_time_ns();")
        measured_sum = source.index("(t_prompt_end - t_start) +")

        self.assertLess(prompt_boundary, profile_readback)
        self.assertLess(profile_readback, generation_boundary)
        self.assertLess(generation_boundary, measured_sum)
        self.assertIn("t_end - t_generation_start", source)


if __name__ == "__main__":
    unittest.main()
