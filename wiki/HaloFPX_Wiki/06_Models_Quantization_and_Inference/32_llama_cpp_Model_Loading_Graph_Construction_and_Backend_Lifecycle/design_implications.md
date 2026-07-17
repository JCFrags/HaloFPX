---
section_id: "32"
title: "Lifecycle integration implications"
status: "needs-machine-validation"
last_verified: "2026-07-16"
applies_to:
  repositories: ["charlie12345/ROCmFPX", "ggml-org/llama.cpp"]
  software_versions: ["a5605a7", "788e07d"]
  hardware_revisions: []
related_sections: ["39", "41", "57", "58", "61", "68"]
---

# Minimal-churn integration map

| HaloFPX concern | Preferred seam | Reason / guardrail |
|---|---|---|
| instrumentation | existing progress/eval callbacks, scheduler split/tensor-backend inspection, server metrics | additive and low risk; record graph signature, placement, copies, bytes and timings |
| persistent prefix restore | above architecture-specific cache/recurrent state objects, before decode | validate fingerprint and shape first; corrupt/mismatch means recompute |
| rank-local ownership | explicit placement plan translated into tensor buffer overrides/scheduler constraints | record rank owner and single-node fallback; do not infer ownership after allocation |
| distributed buffers | new backend/buffer type implementing ggml backend contracts | keeps scheduler visibility; requires lifetime, synchronization, error and reconnect semantics |
| persistent rank graphs | context-level graph cache keyed by exact graph signature | graph contains tensor/backend bindings; invalidate on shape, slot, cache, model, backend or topology change |
| server session lifecycle | slot/session layer with engine-state adapter | keeps admission/cancellation/API behavior separate from tensor state |

**[RECOMMENDATION]** Phase 1 should be trace-only. Phase 2 adds validated rank-local state export/import. Phase 3 adds distributed buffers. Graph persistence follows only after traces prove stable signatures and upstream reserve behavior is understood.

**[RECOMMENDATION]** Every distributed request names coordinator, rank ownership, timeouts, cancellation, partial failure, cleanup, and single-node fallback. A failed remote rank must not leave a slot with partially accepted cache state.

**[INFERENCE]** Reusing an allocated graph can reduce host construction/reserve churn, but it cannot be assumed to improve token latency until measured; backend kernels and memory traffic may dominate.

