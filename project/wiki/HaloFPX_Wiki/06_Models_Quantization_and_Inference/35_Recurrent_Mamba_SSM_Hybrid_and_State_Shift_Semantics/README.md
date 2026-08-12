---
section_id: "35"
title: "Recurrent, Mamba, SSM, Hybrid, and State-Shift Semantics"
status: "needs-machine-validation"
last_verified: "2026-07-16"
applies_to:
  repositories: ["ggml-org/llama.cpp@788e07dc91d266ad3162a1ce9037665656269689", "fewtarius/CachyLLama@6be745998f568e379ea197fcf827baec73ff9940"]
  software_versions: []
  hardware_revisions: ["dual Strix Halo"]
related_sections: ["32", "57", "58", "61", "77"]
---

# 35 - Recurrent and hybrid state semantics

Recurrent state is continuation state, not ordinary attention KV. A checkpoint is valid only when model, graph, state schema, sequence position, and all attention/recurrent components agree.

- **[VERIFIED]** Mamba converts its state-space layer to a recurrent update for autoregressive inference, while retaining a parallel scan for training/prefill [S35-03].
- **[VERIFIED]** Jamba interleaves Transformer attention, Mamba, and MoE blocks, so a continuation can require both attention KV and recurrent state [S35-04].
- **[VERIFIED]** Current llama.cpp implements a dedicated recurrent memory object with sequence operations and state serialization [S35-01].
- **[RECOMMENDATION]** HaloKV must treat hybrid state as one atomic checkpoint manifest. Partial restoration is a cache miss, never accepted continuation.
- **[OPEN]** Exact correctness across context mutation, sequence copy, and shift remains model- and commit-specific and requires adversarial tests.

