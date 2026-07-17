---
section_id: "34"
title: "MoE design implications"
status: "needs-machine-validation"
last_verified: "2026-07-16"
applies_to:
  repositories: ["ggml-org/llama.cpp@788e07dc91d266ad3162a1ce9037665656269689"]
  software_versions: []
  hardware_revisions: ["dual Strix Halo"]
related_sections: ["44", "47", "52", "73"]
---

# Design implications

## Telemetry contract

**[RECOMMENDATION]** Emit one versioned record per evaluation interval with:

| Field family | Required values |
|---|---|
| Identity | model/GGUF hash, llama.cpp commit, quantization, layer, router policy, total/used/shared/group counts |
| Workload | request class, prompt/decode phase, batch and microbatch size, sequence/slot ID pseudonym, token position bucket |
| Routing | selected expert IDs and weights, top-k order, per-layer token counts, pair/co-selection counts |
| Distribution | frequency, weighted frequency, EWMA, entropy, Gini, max/mean, top-N cumulative share, churn and consecutive-token overlap |
| Cost | expert bytes/type/shape, kernel duration, queue wait, transfer bytes/time, cache hit/miss, rank owner |
| Quality | dropped/overflow tokens if applicable, router errors, output-equivalence test ID |

**[RECOMMENDATION]** Aggregate raw token records quickly and retain privacy-safe histograms. Use confidence intervals or repeated-window stability, not a single request.

## Placement rule

**[INFERENCE]** Replication is beneficial only when avoided remote access/transfer latency exceeds extra memory pressure, synchronization, and placement-management cost. Co-selection matters because experts that are hot individually may rarely be active in the same layer/batch.

**[RECOMMENDATION]** Rank candidate experts by measured avoided-cost score:

`benefit = remote_uses * remote_cost - replica_bytes_pressure - update/control_cost`

This is a planner input, not a universal formula. Validate against full end-to-end throughput and tail latency.

**[RECOMMENDATION]** Keep shared experts local on every executing rank unless a measurement disproves it. Prefer whole-layer placement as the correctness baseline; introduce expert-level sharding only behind a single-node fallback and deterministic owner map.

**[ASSUMPTION]** The dual-link fabric can carry expert activations or weights with useful latency. Section 52 must measure this before any MoE-aware topology is promoted.

