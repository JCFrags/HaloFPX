---
section_id: "38"
title: "Distributed Runtime Goals, Cost Model, and Mode Selection"
status: "needs-machine-validation"
last_verified: "2026-07-17"
applies_to:
  repositories: ["charlie12345/ROCmFPX@a5605a72768c6562241b248e268e33dc92787394", "ggml-org/llama.cpp@788e07dc91d266ad3162a1ce9037665656269689"]
  software_versions: []
  hardware_revisions: ["two matched AMD Strix Halo systems; exact revisions open"]
related_sections: ["39", "40", "41", "42", "43", "44", "47", "48", "51"]
---

# 38 - Distributed Runtime Goals, Cost Model, and Mode Selection

**[RECOMMENDATION]** Use full replication as the initial operational baseline. Admit a coupled mode only when a matched, repeated workload shows a p99 objective or model-capacity benefit that outweighs its transfer, synchronization, cache, queueing, and failure costs.

**[MEASURED]** A predecessor RPC configuration is currently operational: nimo-2 coordinates a 121.86 GB model with local ROCm0 plus remote RPC0 on nimo-1, an explicit layer split of `1,1`, and one MPTCP connection using two USB4 subflows [S38-L01]. This proves the baseline can load and become healthy; it does not establish output correctness, throughput, a 200–230 GB capacity, or the fastest mode.

**[OPEN]** No matched HaloFPX latency, throughput, power-under-load, cache-hit, jitter, or mode-comparison experiment exists. Consequently, no mode is declared fastest.

## Authoritative pages

- [Facts and constraints](facts_and_constraints.md) defines the cost terms and source-backed constraints.
- [Design implications](design_implications.md) defines the provisional selector and break-even rules.
- [Procedures and checks](procedures_and_checks.md) defines the required two-node experiment.
- [Open questions](open_questions.md) is the unresolved decision ledger.
- [Sources](sources.md) records pinned evidence.

## Scope boundary

This section selects a mode. Sections 39-48 own protocol, scheduling, mapping, and recovery details. Fabric section 51 must supply measured transport tails. A source implementation is evidence about that implementation, not evidence that HaloFPX already implements the behavior.

## Improvement review

- Correctness: recommendations are separated from upstream facts.
- Freshness: fast-moving repositories are pinned to commits observed on 2026-07-16.
- Main gap: matched two-node p50/p95/p99 data and explicit product SLOs.
- Review trigger: any transport, scheduler, model-format, or runtime commit change; or completion of experiment `DR-38-E1`.
