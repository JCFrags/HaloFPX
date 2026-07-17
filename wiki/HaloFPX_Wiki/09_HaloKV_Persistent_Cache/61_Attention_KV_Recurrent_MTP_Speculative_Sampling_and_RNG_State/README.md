---
section_id: "61"
title: "Attention KV, Recurrent, MTP, Speculative, Sampling, and RNG State"
status: "needs-machine-validation"
last_verified: "2026-07-17"
applies_to:
  repositories: ["ggml-org/llama.cpp@788e07dc91d266ad3162a1ce9037665656269689", "fewtarius/CachyLLama@6be745998f568e379ea197fcf827baec73ff9940"]
  software_versions: []
  hardware_revisions: ["dual Strix Halo"]
related_sections: ["35", "36", "57", "58", "63", "77"]
---

# 61 - Continuation state inventory

Exact continuation requires more than model KV bytes. HaloKV must version independent state streams and publish them as one atomic generation.

- **[VERIFIED]** llama.cpp serializes its active memory implementation; hybrid memories write attention and recurrent components [S61-01].
- **[VERIFIED]** CachyLLama v3 checkpoint records contain target-context, optional draft-context, and speculative-implementation blobs [S61-02].
- **[VERIFIED]** CachyLLama's SSD record has no explicit sampler/grammar/RNG blob field at the pinned commit [S61-02].
- **[RECOMMENDATION]** Treat absent, unknown, corrupt, or mismatched required streams as a cache miss and recompute.
- **[OPEN]** Bit-exact stochastic continuation after process restart is not established.

## Research split

- **Internet/source-code research completed:** pinned llama.cpp memory/state APIs and pinned CachyLLama target, draft, speculative, sampler, and grammar surfaces were inspected; the results describe those exact commits only.
- **Target-machine work required:** inventory every mutable stream for each admitted model/mode, then run exact-continuation and per-stream fault matrices on both Strix Halo hosts.
- **Contingent decisions:** required versus reconstructible streams, persistent sampler/RNG representation, partial-reuse policy, and any exact-continuation promise remain unapproved until those results exist.
