---
section_id: "40"
title: "Replication Facts and Constraints"
status: "needs-machine-validation"
last_verified: "2026-07-16"
applies_to:
  repositories: ["vllm-project/vllm@9354f222042986addf20709e5274fc26e0d09745", "ggml-org/llama.cpp@788e07dc91d266ad3162a1ce9037665656269689", "fewtarius/CachyLlama@6be745998f568e379ea197fcf827baec73ff9940"]
  software_versions: []
  hardware_revisions: []
related_sections: ["38", "46", "48", "55"]
---

# Facts and constraints

- **[VERIFIED]** vLLM documents each data-parallel engine as having an independent KV cache and identifies running queue, waiting queue, and KV state as routing signals [S40-01]. Its current internal load balancer is queue-based, with KV-aware logic described as a future sophistication at the pinned commit.
- **[VERIFIED]** `llama.cpp` server supports parallel slots, continuous batching, prompt-cache reuse, explicit slot selection, and slot save/restore endpoints [S40-02]. Its documentation warns cache reuse can be nondeterministic on backends where logits vary by batch size.
- **[VERIFIED]** CachyLlama contains SSD KV-cache and server cache/page-management code at commit `6be7459` [S40-03]. This proves code presence, not crash consistency, cross-node portability, or HaloFPX performance.

## Capacity accounting

For replica `r`, expose:

- loaded model identity and supported API/features;
- usable KV bytes/pages, active sequences, maximum sequence/batch limits;
- running and waiting token counts, not request count alone;
- estimated prefill/decode work and earliest deadline slack;
- cache residency/longest-prefix match for a privacy-safe session or prefix key;
- recent phase-specific service-time p50/p95/p99 and health;
- memory high-water and cache eviction pressure.

**[INFERENCE]** Request count is an inadequate load signal because prompt length, generated length, and cache reuse vary. Token work plus observed service curves is a better predictor; this follows from iterative serving behavior [S40-04].

## Cache and session constraints

**[RECOMMENDATION]** A session directory entry is `(session_id, epoch, model_hash, tokenizer_hash, sampler_abi, primary_replica, optional_checkpoint_replica, committed_position, expiry)`. A replica cache entry is rank-local unless its exact compatibility identity and transfer state prove otherwise.

Affinity is a preference, not a correctness requirement. If the owner is overloaded, missing, stale, or incompatible, the coordinator may recompute from authoritative tokens on another replica. A cache mismatch/corruption is a miss, never accepted state.

## Model placement patterns

| Placement | Benefit | Cost/constraint |
|---|---|---|
| same model both nodes | failover and aggregate concurrency | duplicate weights; no larger single model |
| different models | model diversity | no failover unless alternate also fits/loads |
| primary plus warm standby | lower recovery time | idle capacity and duplicate memory |
| dynamic reload | flexible catalog | load latency, cache loss, storage contention |
