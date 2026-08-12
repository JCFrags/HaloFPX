---
section_id: "40"
title: "Full Replication, Request Routing, and Session Affinity"
status: "needs-machine-validation"
last_verified: "2026-07-16"
applies_to:
  repositories: ["ggml-org/llama.cpp@788e07dc91d266ad3162a1ce9037665656269689", "fewtarius/CachyLlama@6be745998f568e379ea197fcf827baec73ff9940"]
  software_versions: []
  hardware_revisions: ["dual Strix Halo premise"]
related_sections: ["38", "39", "46", "48", "55", "56"]
---

# 40 - Full Replication, Request Routing, and Session Affinity

**[RECOMMENDATION]** Make two independent full-model servers behind a cache-aware coordinator the first usable distributed mode. Route an established session to its current cache owner while healthy and within a queue guard; route new/cache-cold work by predicted completion time, not round-robin alone.

Replication maximizes failure isolation and independent concurrency but duplicates model memory and does not let one request use both machines. Failover preserves service only if the model exists on the survivor; warm-session continuation additionally needs a compatible cache/checkpoint or prompt recomputation.

Pages: [facts](facts_and_constraints.md), [design](design_implications.md), [procedures](procedures_and_checks.md), [open questions](open_questions.md), [sources](sources.md).

## Improvement review

Routing state, ownership, and failover semantics are explicit. Numeric queue/cache weights and failover SLOs remain unmeasured.
