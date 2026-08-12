---
section_id: "76"
title: "Distributed Benchmark Design Implications"
status: "needs-machine-validation"
last_verified: "2026-07-16"
applies_to:
  repositories: ["ggml-org/llama.cpp@788e07dc91d266ad3162a1ce9037665656269689"]
  software_versions: []
  hardware_revisions: ["planned matched dual Strix Halo"]
related_sections: ["38", "40", "41", "42", "43", "44", "47", "73", "74", "75"]
---

# Design Implications

## Matched matrix

**[RECOMMENDATION]** Cross these core dimensions in staged blocks:

- model family and exact model/quant hashes, including dense, MoE, hybrid/recurrent, and MTP-capable models where supported;
- short/medium/long prompt and decode lengths, context occupancy, cold/warm HaloKV state;
- concurrency 1, 2, 4, 8 and a measured saturation sweep;
- deterministic greedy and a fixed stochastic sampling profile;
- single rank 0, single rank 1, two replicas, native MTP, local draft control, remote draft, tensor, pipeline, and MoE hybrid;
- link A, link B, selected two-link policies, and degraded single-link operation;
- idle and sustained power/thermal states.

Use the same request trace for paired cells. Separate capacity tests from latency-under-SLO tests.

## Break-even formulas

**[RECOMMENDATION]** Use observed components rather than theoretical FLOPs alone.

- Remote speculation wins per target cycle only where `T_remote_draft + T_transfer_wait + T_verify + T_recovery < T_baseline_for_equivalent_emitted_tokens`.
- Two-rank tensor parallel wins where saved compute exceeds collective, smaller-kernel, imbalance, and synchronization overhead.
- Pipeline steady-state capacity is bounded by its slowest stage plus boundary transfer; single-request latency still includes all stages. Report measured fill/drain idle fraction.
- MoE hybrid wins where avoided expert weight/memory work exceeds routed-token transfer, imbalance, and cold-expert penalties.
- Replication wins a workload objective where queueing reduction and failure isolation outweigh duplicated memory/cache and any routing cost.

These are **[INFERENCE]** templates, not predictive results.

## Statistical decision rule

**[RECOMMENDATION]** For each workload region compute paired ratios against the matched control and a confidence interval using the Section 73-approved method. Call a mode beneficial only when:

1. correctness/quality gates pass;
2. the lower confidence bound exceeds `1 + epsilon` for the declared objective;
3. p95/p99 latency, power, memory, and failure budgets pass;
4. the result repeats on both cable directions and relevant thermal states;
5. the result remains beneficial under a predeclared sensitivity margin.

`epsilon` and confidence level are **[OPEN]** project policy. Publish inconclusive regions instead of interpolating a winner.

## Plan output

**[RECOMMENDATION]** The planner consumes a versioned table keyed by exact model, quant, context bucket, concurrency bucket, cache state, power profile, fabric profile, and objective. It returns the selected mode, expected interval, validity range, fallback, evidence run IDs, and expiry trigger. Out-of-domain inputs fall back to a verified conservative mode.

## Failure degradation

Every distributed plan declares rank ownership, link dependence, timeout boundary, cancellation semantics, and single-node fallback. Failure tests measure detection time, lost/incomplete requests, state validity, recovery time, and post-recovery correctness—not only throughput after restart.
