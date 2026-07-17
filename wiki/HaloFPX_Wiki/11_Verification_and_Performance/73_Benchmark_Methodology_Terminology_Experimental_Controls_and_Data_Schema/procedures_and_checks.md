---
section_id: "73"
title: "Benchmark Procedures and Validation Checks"
status: "needs-machine-validation"
last_verified: "2026-07-17"
applies_to:
  repositories: ["HaloFPX integration repository (not yet frozen)"]
  software_versions: ["proposed protocol v0.1"]
  hardware_revisions: ["dual-Strix-Halo target"]
related_sections: ["18", "22", "27", "55", "65", "74", "75", "76", "77", "78", "79", "81", "84"]
---

# Benchmark procedures and validation checks

## Prerequisites

**[RECOMMENDATION]** Run from an immutable benchmark checkout. Root is not required for the load generator, but may be required for performance governor, IRQ, cache-drop, and hardware telemetry controls. Never drop caches, change clocks, or stop services on a shared machine without recording and authorizing the state change.

## 1. Freeze the experiment card

Before executing, write a manifest containing:

- hypothesis, independent variable, primary metric, secondary metrics, and stop rule;
- exact source/build SHAs and dirty patch hash;
- model, tokenizer, prompt-set, plan, and schema SHA-256 hashes;
- all controls listed in [facts and constraints](facts_and_constraints.md#controls-and-matching-constraints);
- warmup/reset procedure, repetition count, run order seed, and confidence method;
- SUT boundary and power boundary;
- expected failure, timeout, and retry handling.

**[RECOMMENDATION]** Abort before measurement if an identity cannot be captured. Do not fill it in later from memory.

## 2. Establish time and telemetry validity

On both nodes, capture current clock source, synchronization status, UTC time, kernel, device topology, and sensor inventory. Representative read-only commands (availability is machine-dependent):

```bash
date --iso-8601=ns
timedatectl show --property=NTPSynchronized --property=Timezone
uname -a
lspci -nn
amd-smi version
amd-smi monitor --power --temperature --usage --mem-usage --csv
```

**[RECOMMENDATION]** Record the exact AMD SMI version and command because fields change across releases. Confirm cadence and units from output; do not assume them. [S73-09]

For cross-node event timing, measure offset/uncertainty before and after the run. **[OPEN]** Select and validate the local PTP/NTP procedure in sections 55/75; `NTPSynchronized=yes` alone is not a latency error bound.

## 3. Prepare workload and run order

1. Tokenize prompts once with the pinned tokenizer and preserve input-token counts.
2. Generate the arrival trace and requested output lengths from a recorded seed.
3. Construct paired blocks for each A/B condition.
4. Randomize condition order within each block.
5. Predeclare whether EOS is honored or output length is forced.
6. Keep sampling settings and seed identical where deterministic comparison is intended.

**[RECOMMENDATION]** Include at least fixed-length synthetic bins and a preserved representative prompt set. Synthetic tests isolate scaling; representative tests expose scheduler, EOS, and content effects. Do not merge them into one aggregate.

## 4. Warmup and steady state

**[RECOMMENDATION]** Separate warmup from measurement. Initial exploratory default:

- load the model and execute at least three unrecorded workload-equivalent requests;
- continue warmup until five consecutive windows have primary-metric medians within 3% of their combined median and no monotonic temperature rise above sensor noise;
- cap warmup and mark the run `steady_state_not_reached` rather than warming indefinitely;
- record all warmup observations, but exclude them from the analysis population.

**[OPEN]** The `3%`/five-window rule is a proposed starting point, not a verified HaloFPX threshold. Section 79 must replace it with an evidence-backed rule.

## 5. Repetitions and sample budget

**[RECOMMENDATION]** For exploratory A/B work, use at least five independent paired steady-state runs per condition. For a release claim:

1. run a pilot to estimate across-run variance;
2. choose the minimum detectable regression and power/coverage target before the confirmatory run;
3. calculate the required independent run and request counts;
4. use a fixed count or a predeclared sequential rule;
5. never stop because the current result became favorable.

**[VERIFIED]** MLPerf publishes large observation requirements for reliable tail-latency statements; this supports sample planning, not copying its thresholds into HaloFPX without its complete protocol. [S73-04]

## 6. Raw capture and schema validation

The validated request fixture uses this shape:

```json
{
  "record_model_version": "1.0.0",
  "record_type": "request",
  "run_id": "019f6ece-b364-79c2-8c40-e9c025d9499d",
  "recorded_at": "2026-07-17T12:00:00Z",
  "monotonic_clock": "CLOCK_MONOTONIC_RAW",
  "monotonic_ns": 1000000000,
  "request_id": "fixture-request-001",
  "status": "success",
  "input_tokens": 128,
  "output_tokens": 4,
  "request_start_ns": 1000000000,
  "first_output_observed_ns": 1250000000,
  "final_response_observed_ns": 1400000000,
  "error_code": null,
  "cache": {"eligible_tokens": 64, "restored_tokens": 64, "class": "exact_hit"},
  "speculation": {"proposed_tokens": 3, "accepted_tokens": 2, "target_only_tokens": 2}
}
```

**[RECOMMENDATION]** Integers in the abbreviated object above are illustrative, not measurements. Use the checked-in [valid fixture](data/fixtures/valid-request.json), [invalid fixture](data/fixtures/invalid-request.json), and validator rather than copying the abbreviated object.

From the repository root, run:

```powershell
$section = 'wiki/HaloFPX_Wiki/11_Verification_and_Performance/73_Benchmark_Methodology_Terminology_Experimental_Controls_and_Data_Schema'
python "$section/scripts/validate_benchmark_records.py" "$section/data/benchmark_record.schema.json" "$section/data/fixtures/valid-request.json" --expect valid
python "$section/scripts/validate_benchmark_records.py" "$section/data/benchmark_record.schema.json" "$section/data/fixtures/invalid-request.json" --expect invalid
```

**[VERIFIED]** On Python 3.14.4 with `jsonschema` 4.26.0, both expectation commands exited zero; the valid fixture had zero errors and the invalid fixture was rejected with nine structural/semantic errors. Commands, hashes, and summarized results are preserved in [schema-validation-result.json](evidence/schema-validation-result.json) [S73-11].

**[RECOMMENDATION]** Validate each JSONL record individually before derivation. Quarantine invalid records without repair. Schema validation does not approve statistical choices or convert a structurally valid record into a `[MEASURED]` claim.

## 7. Derivation and statistics

1. Reject structurally invalid records into a quarantine report; do not silently repair.
2. Verify monotonic timestamp ordering and nonnegative counts.
3. Calculate TTFT, request latency, observed ITL, TPOT, throughput, cache rates, acceptance, and energy only from their declared populations.
4. Retain failures and report offered, completed, successful, and goodput denominators.
5. Compute per-run estimates first.
6. Report median, p90, p95, p99 using the named quantile method (initial recommendation: NumPy linear/type-7 equivalent).
7. Report two-sided 95% intervals with the predeclared method and seed.
8. Emit a derivation receipt containing script SHA, runtime/library versions, raw input hashes, and output hashes.

## 8. Comparison checklist

- [ ] Independent variable is singular or interactions are explicitly modeled.
- [ ] Model/tokenizer/prompt/plan hashes match.
- [ ] Batch, context, sampling, arrival trace, and output policy match.
- [ ] Warmup and reset states match.
- [ ] Cache tier and compatibility outcome are proven, not inferred from timing.
- [ ] Rank/world/link topology matches or is the variable under test.
- [ ] Both nodes' failures, power, and telemetry are included for distributed modes.
- [ ] Client-visible metrics and engine-only metrics are not mixed.
- [ ] Tail populations and sample counts are shown.
- [ ] Raw artifact and schema validation receipt exist.
- [ ] No `[MEASURED]` label appears without linked raw data and environment metadata.

## 9. Internet follow-up

1. Pin the final benchmark dependencies and archive their licenses/source snapshots.
2. Compare vLLM and NVIDIA metric formulas again at implementation time; their current state is volatile. [S73-02][S73-03]
3. Track MLCommons inference-policy revisions for sample/tail methodology changes without claiming conformance. [S73-04]
4. Review AMD SMI changelogs for telemetry field/unit changes before freezing the parser. [S73-09]
5. Review the chosen statistical library's exact quantile and bootstrap algorithms and pin its version.

## 10. On-machine validation tasks

1. Characterize cold-start and steady-state convergence for each backend/model bin.
2. Estimate run-to-run variance and autocorrelation under controlled idle and sustained-load states.
3. Validate token timestamps against packet/server traces and chunk bundling.
4. Calibrate device telemetry against synchronized wall power.
5. Prove DRAM, page-cache, and NVMe cache-state classifications.
6. Measure dual-node clock offset/uncertainty and collective event correlation.
7. Inject request failures/timeouts and verify denominator retention.
8. Round-trip schema validation and reproduce summaries from raw hashes.
