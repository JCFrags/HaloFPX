---
section_id: "33"
title: "Attention and cache design implications"
status: "needs-machine-validation"
last_verified: "2026-07-16"
applies_to:
  repositories: ["charlie12345/ROCmFPX", "ggml-org/llama.cpp"]
  software_versions: ["a5605a7", "788e07d"]
  hardware_revisions: []
related_sections: ["35", "42", "57", "58", "61"]
---

# Design implications

- **[RECOMMENDATION]** Cache descriptor fields: model/GGUF hashes, architecture and layer map, K/V/recurrent tensor types, exact shapes/strides/block sizes, token positions and sequence IDs, RoPE/scaling, sliding-window policy, FA mode, runtime commit, backend, rank/topology and checksum.
- **[RECOMMENDATION]** Begin with F16 or Q8 K/V as correctness references. Qualify lower-bit types per model/context with KLD/PPL and task tests before persistence.
- **[RECOMMENDATION]** Test asymmetric cache types explicitly. K participates in attention-score estimation while V is aggregated after softmax; this motivates—but does not prove—that K may need more protection.
- **[RECOMMENDATION]** Treat TurboQuant K/V, symmetric TurboQuant, and boundary-layer policies as experiments. Do not inherit the fork's default from one model/environment.
- **[INFERENCE]** For two-way tensor parallel GQA, co-locating each KV group with its query heads minimizes KV duplication, while output projection still needs a collective. Pipeline parallel naturally keeps each layer's cache rank-local.
- **[INFERENCE]** MLA needs an architecture-specific compressed-latent cache schema and may reduce transport/state size, but only the exact graph/tensors define what is persisted.
- **[RECOMMENDATION]** Global/sliding layers use separate segment classes so retention, eviction, restore and corruption checks match semantics.
- **[RECOMMENDATION]** Recurrent state and KV state commit atomically at a token boundary across ranks. Partial rank writes are invalid generations.

Single-node fallback must reconstruct or load all rank-owned state locally. If that is impossible, reject before modifying slot state.

