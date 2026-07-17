---
section_id: "44"
title: "MoE Hybrid Design Implications"
status: "needs-machine-validation"
last_verified: "2026-07-16"
applies_to:
  repositories: ["charlie12345/ROCmFPX@a5605a72768c6562241b248e268e33dc92787394", "ggml-org/llama.cpp@788e07dc91d266ad3162a1ce9037665656269689"]
  software_versions: ["vLLM@9354f222042986addf20709e5274fc26e0d09745", "Megatron-LM@740c16e6b80a753bea26232148d9bb2d7f0c827a", "DeepEP@dd758caf451848bd150e1046af3d0a73e5fff38d"]
  hardware_revisions: ["dual AMD Strix Halo; exact BOM and USB4 fabric measurements unresolved"]
related_sections: ["34", "38", "42", "43", "45", "47", "52", "53"]
---

# Design implications

## Recommended plan hierarchy

1. **[RECOMMENDATION] Correctness baseline:** run the model on one node when it fits, or use contiguous pipeline stages with each MoE layer wholly owned by one rank. This provides a reference with no intra-layer remote expert dispatch.
2. **[RECOMMENDATION] Static hybrid:** keep a stable model-specific owner map for cold experts. Replicate routers on ranks that hold the corresponding token activations. Replicate shared experts only when both ranks compute that MoE layer and memory permits.
3. **[RECOMMENDATION] Hot-replica optimization:** add byte-identical physical copies of selected logical experts under a fixed memory budget, then deterministically divide assignments among replicas.
4. **[RECOMMENDATION] Dynamic rebalancing:** defer until static placement wins end to end and asynchronous weight preparation plus atomic map activation is proven.

This ordering keeps unsupported dynamism out of the first implementation while preserving the vLLM/FlexMoE logical-versus-physical expert model [S44-02][S44-08].

## Attention sharding versus layer pipeline

| Question | Attention-sharded / expert-parallel layer | Contiguous layer pipeline |
|---|---|---|
| Router placement | replicated with each token-owning rank; identical weights and transforms | colocated with the layer owner |
| Routed experts | static owners, optional replicas across both nodes | local to the stage owner |
| Communication | dispatch and combine at every distributed MoE layer | hidden-state transfer at stage boundary |
| Load-balancing opportunity | fine-grained; can use both GPUs within a layer | stage/microbatch balance only |
| Decode risk | many small variable exchanges and barriers | pipeline bubble and stage imbalance |
| Memory | partitions cold experts but needs packing/workspace/replicas | each stage holds complete experts for its layers |
| Failure | missing remote cold expert invalidates step | missing stage invalidates step |

- **[INFERENCE]** Attention/EP hybrid is attractive when cold expert weights do not fit per stage or routing skew can be exploited, but it creates a link-critical path at each distributed MoE layer.
- **[INFERENCE]** Pipeline is attractive when a balanced layer cut fits both nodes because its transport is regular and graph topology is simpler.
- **[RECOMMENDATION]** The topology planner must compare both using measured phase-specific cost, not select EP merely because the model is sparse.

## Router and shared-expert placement

- **[RECOMMENDATION] Router:** compute routing where the current hidden states reside. If hidden states are sharded in a way that prevents exact router logits locally, perform the required collective before top-k; do not approximate the router.
- **[RECOMMENDATION] Shared experts:** treat them as an always-required dense branch according to the model graph. When both ranks process tokens, replicating shared experts avoids an extra remote branch but consumes memory. Megatron's overlap is useful design evidence, yet its CUDA/NCCL implementation is not directly portable [S44-03].
- **[RECOMMENDATION] Combine:** the token-origin rank owns final weighted reduction unless a model-specific graph deliberately transfers ownership. Combine must wait for all selected routed outputs and the shared branch.

## Hotness, placement, and replica policy

Track at least `(model, layer, logical_expert, workload_class, phase)` with token-assignment count, bytes, compute time, queue time, remote fraction, and observation window.

- **[RECOMMENDATION]** Separate prefill and decode profiles; do not allow a high-volume prefill window to hide decode imbalance.
- **[RECOMMENDATION]** Candidate hotness requires repeated windows, minimum assignments, and a benefit estimate larger than copy/amortization cost plus hysteresis.
- **[RECOMMENDATION]** Optimize the slowest-rank critical path, not only global expert frequency. A frequently selected expert already local to the bottleneck-free rank may not deserve a replica.
- **[RECOMMENDATION]** Allocate replicas per layer under exact byte budgets. vLLM documents why one logical replica across every MoE layer can consume substantial memory [S44-02].
- **[RECOMMENDATION]** Use deterministic replica selection from current queue/load plus a stable tie-breaker. Replica choice may change execution location, never logical expert ID or router weight.

## Reconfiguration protocol

An owner map is immutable within a plan epoch.

1. coordinator derives a candidate from a closed telemetry window;
2. both ranks validate memory and supported tensor/backend layouts;
3. new weights are copied to inactive physical slots and checksummed;
4. ranks acknowledge readiness for the same manifest digest;
5. scheduler drains to a batch/step boundary;
6. coordinator atomically publishes the new epoch;
7. old slots remain until all work from the old epoch retires, then may be freed.

- **[RECOMMENDATION]** Reject a candidate if either rank cannot stage it with safety headroom.
- **[RECOMMENDATION]** Bind commands and buffers to `plan_epoch`; section 45 should reject stale-epoch dispatch.
- **[RECOMMENDATION]** Rate-limit changes and require an estimated amortization horizon. Oscillating placement is a correctness and latency risk.

## Hidden-vector handoff versus token all-to-all

- **[INFERENCE] Coarse hidden-vector handoff** has predictable shape and few synchronization points. It can waste compute/memory balance opportunities because an entire stage owns all experts.
- **[INFERENCE] Token-wise all-to-all** sends only routed assignments to owners and enables cold partitioning/hot replication, but message size and destination mix are data-dependent. Top-k may multiply activation traffic unless packing coalesces assignments by destination.
- **[RECOMMENDATION]** Implement explicit wire accounting: payload, routing metadata, padding, retries, and both dispatch/combine directions. Compare p50/p95/p99, not payload bytes alone.
- **[RECOMMENDATION]** Prefer the coarse path when the fine-grained plan does not beat it with the same model, prompts, cache state, batching, sampling, and thermal envelope.

## Single-node degraded mode

**[RECOMMENDATION]** Advertise single-node continuation only when a complete compatible model plan is resident or can be reloaded and state replay is validated. If peer-only experts are required, mark the session unavailable and preserve the last safe checkpoint; do not present hot-replica coverage as full redundancy.
