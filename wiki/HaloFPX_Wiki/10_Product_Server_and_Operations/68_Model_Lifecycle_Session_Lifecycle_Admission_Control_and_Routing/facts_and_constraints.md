---
section_id: "68"
title: "Lifecycle Facts and Invariants"
status: "needs-machine-validation"
last_verified: "2026-07-16"
applies_to:
  repositories: ["ggml-org/llama.cpp", "fewtarius/CachyLLama", "HaloFPX project"]
  software_versions: ["llama.cpp 788e07d", "CachyLLama 6be7459"]
  hardware_revisions: ["two-node Strix Halo target"]
related_sections: ["38", "39", "40", "45", "46", "48", "60", "61", "67"]
---

# Facts and invariants

## Verified source facts

- **[VERIFIED]** Pinned `llama-server` can run as a multi-model router, discovering cached models, a model directory, or presets and forwarding requests to model instances [S68-01].
- **[VERIFIED]** It documents dynamic model loading/unloading, multi-shard directory layout, warmup enabled by default, parallel slots, continuous batching, prompt-cache controls, and sleep-on-idle that unloads model and KV memory [S68-01].
- **[VERIFIED]** Its health endpoint distinguishes loading (503) from ready (200), but that upstream state is not sufficient to prove all distributed ranks/links are ready [S68-01].
- **[VERIFIED]** CachyLLama documents per-user concurrency caps returning 429, user-scoped checkpoints, and slot affinity for cache locality [S68-02].
- **[VERIFIED]** Those upstream behaviors do not define HaloFPX’s distributed session epoch, shard admission, rank failure, or safe migration semantics.

## Lifecycle invariants

1. **[RECOMMENDATION]** A model is routable only after artifact/shard validation, resource reservation, all required rank loads, warmup, and plan readiness pass.
2. **[RECOMMENDATION]** A session has one coordinator owner and monotonically increasing epoch; ranks reject stale commands/results.
3. **[RECOMMENDATION]** Active generation is never migrated. A continuation may be re-admitted on another compatible plan only from a committed token boundary and with validated/restored state or clean recomputation.
4. **[RECOMMENDATION]** Memory reservation includes model/shards, runtime graph/workspaces, KV/recurrent state, transport buffers, cache staging, and safety headroom.
5. **[RECOMMENDATION]** Draining stops new admissions while allowing bounded active requests to finish or cancel.
6. **[RECOMMENDATION]** Expiry/eviction cannot delete rank state still referenced by an active epoch.
7. **[RECOMMENDATION]** A fallback is eligible only if its exact model/quality/API semantics meet the request and its manifest is already admitted.

## Unproven assumptions

- **[ASSUMPTION]** At least one important model fits on a single node for degraded service.
- **[ASSUMPTION]** Stable agent prefixes make session/cache affinity valuable enough to influence routing.
- **[OPEN]** Actual load peaks, warmup cost, model residency set, queue load, and session retention are unknown.

