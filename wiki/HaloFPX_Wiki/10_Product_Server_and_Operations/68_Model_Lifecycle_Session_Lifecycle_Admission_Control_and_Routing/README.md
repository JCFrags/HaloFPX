---
section_id: "68"
title: "Model Lifecycle, Session Lifecycle, Admission Control, and Routing"
status: "needs-machine-validation"
last_verified: "2026-07-16"
applies_to:
  repositories: ["HaloFPX project", "ggml-org/llama.cpp", "fewtarius/CachyLLama"]
  software_versions: ["llama.cpp 788e07d", "CachyLLama 6be7459"]
  hardware_revisions: ["two-node Strix Halo target"]
related_sections: ["07", "09", "38", "39", "40", "45", "46", "47", "48", "60", "61", "66", "67", "69", "72"]
---

# Lifecycle, admission, and routing

**[RECOMMENDATION]** The coordinator owns model admission, session identity/epoch, plan choice, request queueing, and client-visible terminal state. Each rank owns its local model shard, compute state, KV/recurrent state, and persistent-cache objects. Ownership never transfers implicitly.

HaloFPX should expose explicit state machines for models and sessions, reserve resources before load/admission, route continuations for cache locality without sacrificing fairness, and degrade only to compatible prevalidated plans.

See [verified inputs and invariants](facts_and_constraints.md), [state machines and routing policy](design_implications.md), and [fault/lifecycle checks](procedures_and_checks.md).

## Research split

- Source research verified upstream router/model discovery, loading/sleep behavior, slots/caching, and CachyLLama user caps/affinity.
- Machine work must measure load/unload/warmup/reservations, queueing, continuation, expiry, cancellation, routing, and faults.
- Contingent decisions include session durability, migration policy, preloaded model set, queue disciplines, caps, and degraded-plan eligibility.

