---
section_id: "73"
title: "Benchmark Open Questions"
status: "needs-machine-validation"
last_verified: "2026-07-17"
applies_to:
  repositories: ["HaloFPX integration repository (not yet frozen)"]
  software_versions: ["HaloFPX benchmark record schema 1.0.0", "jsonschema 4.26.0"]
  hardware_revisions: ["dual-Strix-Halo target"]
related_sections: ["05", "18", "22", "27", "55", "65", "74", "75", "76", "77", "79", "81", "84"]
---

# Benchmark open questions

| ID | **[OPEN]** Question | Evidence needed | Owner/dependency |
|---|---|---|---|
| OQ-73-01 | What exact warmup convergence rule is valid for each model/backend/mode? | timestamped throughput, temperature, clocks, cache state across repeated cold starts | sections 74, 76, 79 |
| OQ-73-02 | How many independent runs and requests are required for release gates? | pilot variance, tail distribution, minimum meaningful regression, false-positive/negative targets | sections 79, 81 |
| OQ-73-03 | Which clock-sync method yields a bounded cross-node timing error? | before/after offset measurements against a selected PTP/NTP design | sections 55, 75 |
| OQ-73-04 | Which AMD SMI fields are available and stable on the exact ROCm/firmware stack? | versioned JSON/CSV samples and sensor inventory from both nodes | sections 18, 22, 27 |
| OQ-73-05 | What is the wall-power measurement boundary and calibration accuracy? | calibrated meter identity, cadence, uncertainty, synchronized captures | sections 22, 79 |
| OQ-73-06 | Can the streaming stack expose one observation timestamp per token rather than per chunk? | client/server trace comparison under streaming and speculative output | sections 66, 76 |
| OQ-73-07 | What prompt set represents intended interactive, agentic, long-context, and cache-reuse workloads? | workload inventory, license/privacy review, manifest and length/content distributions | sections 07, 84 |
| OQ-73-08 | Should output length be forced or EOS-driven for each benchmark family? | sensitivity study showing effect on throughput and latency | sections 74, 76, 78 |
| OQ-73-09 | What exact quantile and bootstrap implementations become normative? | cross-library test vectors, version pin, reproducibility check | section 81 |
| OQ-73-10 | Which cache events prove NVMe restore rather than DRAM/page-cache reuse? | instrumented tier/path events plus controlled cache-state tests | sections 65, 77 |
| OQ-73-11 | What is the eligibility denominator for cache hit rates across partial prefixes and incompatible records? | finalized HaloKV lookup contract and event taxonomy | sections 57, 58, 65 |
| OQ-73-12 | How should speculative acceptance be attributed with MTP trees or variable proposal depths? | per-step proposal/accept/reject trace and exact decoder semantics | sections 36, 41, 61, 76 |
| OQ-73-13 | Which resource-utilization signals are sufficiently meaningful for gfx1151? | profiler/sensor correlation with controlled kernels | sections 27, 37, 74 |
| OQ-73-14 | How are failed, cancelled, overloaded, and degraded-mode requests represented in goodput gates? | server error-model decision and fault-injection artifacts | sections 66, 80, 81 |
| OQ-73-15 | Where will raw artifacts live, and what retention/privacy limits apply? | section 05 convention plus capacity, access, and redaction decision | sections 05, 71, 84 |

## Decisions blocked by these questions

**[OPEN]** Do not freeze release thresholds, a universal warmup count, a single representative prompt set, a wall-power claim, or a cross-node latency gate until the corresponding evidence above exists.

## Resolved structural question

**[VERIFIED]** OQ-73-16 is resolved for schema 1.0.0: the checked-in Draft 2020-12 schema, deterministic validator, valid/invalid fixtures, and execution receipt define current structural conformance [S73-11]. This does not resolve any metric threshold or sample-sufficiency question.

## Review triggers

**[RECOMMENDATION]** Revisit this section when the first benchmark harness lands, when the frozen HaloFPX base changes, when vLLM/NVIDIA/MLCommons metric definitions materially change, when AMD SMI output changes, or after the first two-node variance study.
