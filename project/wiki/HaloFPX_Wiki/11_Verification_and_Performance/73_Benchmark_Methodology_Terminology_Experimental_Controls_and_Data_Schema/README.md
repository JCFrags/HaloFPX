---
section_id: "73"
title: "Benchmark Methodology, Terminology, Experimental Controls, and Data Schema"
status: "needs-machine-validation"
last_verified: "2026-07-17"
applies_to:
  repositories:
    - "charlie12345/ROCmFPX@a5605a72768c6562241b248e268e33dc92787394"
    - "ggml-org/llama.cpp@788e07dc91d266ad3162a1ce9037665656269689"
  software_versions: ["HaloFPX benchmark record schema 1.0.0", "jsonschema 4.26.0"]
  hardware_revisions: ["two matched AMD Strix Halo systems; exact BOM pending section 18"]
related_sections: ["05", "18", "22", "27", "38", "55", "57", "65", "74", "75", "76", "77", "78", "79", "81", "84"]
---

# 73 - Benchmark Methodology, Terminology, Experimental Controls, and Data Schema

## Purpose and status

**[RECOMMENDATION]** Use this section as the measurement contract for HaloFPX. A result is comparable only when the system under test, workload, token accounting, cache state, sampling, environment, and statistic are explicitly identified. Detailed facts are in [facts and constraints](facts_and_constraints.md); the executable protocol is in [procedures and checks](procedures_and_checks.md).

**[VERIFIED]** Existing tools do not measure identical scopes. Pinned `llama-bench` separates prompt processing and text generation but excludes tokenization and sampling; vLLM serving metrics include client-observed end-to-end latency; NVIDIA's published metric definitions include tokenization/detokenization in TTFT and exclude the terminal empty/done event. Therefore an unlabeled number called "latency" or "tokens/s" is not portable across tools. [S73-01][S73-02][S73-03]

**[RECOMMENDATION]** Every published comparison must include:

1. immutable run, build, model, tokenizer, prompt-set, plan, and schema identities;
2. one raw record per request plus time-series and distributed-operation records;
3. warmup excluded from analysis and separately recorded;
4. at least five independent steady-state repetitions for exploratory comparisons, with a predeclared stopping rule for release claims;
5. median, p90, p95, and p99 for request latency, TTFT, and inter-token latency, plus uncertainty intervals and sample counts;
6. matched settings and randomized or counterbalanced execution order;
7. failed, cancelled, timed-out, and rejected requests retained in denominators rather than silently discarded.

**[VERIFIED]** The checked-in Draft 2020-12 [benchmark record schema](data/benchmark_record.schema.json) structurally covers the seven record families, and the deterministic [validator](scripts/validate_benchmark_records.py) accepts the valid fixture and rejects the invalid fixture with preserved hashes and exit status [S73-11].

**[RECOMMENDATION]** Store append-only records as newline-delimited JSON validated one record at a time; large token-event and telemetry streams may be compressed separately while retaining content hashes. Schema validity proves record structure and selected cross-field invariants only. It does not approve warmup, repetition counts, confidence methods, practical-effect thresholds, or release decisions.

## No benchmark results in this section

**[OPEN]** No HaloFPX measurement, benchmark harness integration, power calibration, or two-node run was available for this research. All numerical run-count, warmup, confidence, and threshold guidance here remains a proposed protocol, not a measured property of either Strix Halo machine.

## Research split

| Track | State | Outcome |
|---|---|---|
| Internet/source-code and schema work | completed for this draft | primary-source conflicts plus schema/validator fixture evidence recorded in [sources](sources.md) and [evidence](evidence/schema-validation-result.json) |
| On-machine research | required | clock validation, sensor inventory, warmup convergence, variance, load profiles, cache path proof, and dual-link telemetry |
| Decisions contingent on measurements | open | final repetition count, warmup rule, tail sample budget, accepted sensor accuracy, and release thresholds |

## Reporting rule in one sentence

**[RECOMMENDATION]** Publish the raw artifact first, then a derived summary whose metric name, scope, unit, population, percentile method, confidence method, failure policy, and provenance are machine-readable; never publish a chart-only result.
