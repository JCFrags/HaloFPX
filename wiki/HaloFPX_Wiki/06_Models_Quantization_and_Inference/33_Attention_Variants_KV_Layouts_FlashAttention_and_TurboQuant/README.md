---
section_id: "33"
title: "Attention Variants, KV Layouts, FlashAttention, and TurboQuant"
status: "needs-machine-validation"
last_verified: "2026-07-16"
applies_to:
  repositories: ["ggml-org/llama.cpp", "charlie12345/ROCmFPX"]
  software_versions: ["llama.cpp 788e07d", "ROCmFPX a5605a7"]
  hardware_revisions: ["gfx1151 validation pending"]
related_sections: ["29", "30", "35", "42", "57", "61"]
---

# Attention and persistent state

KV layout is architecture-, layer-, cache-type-, backend-, context-, slot-, and topology-dependent. **[VERIFIED]** Current upstream accepts F32/F16/BF16/Q8_0/Q4_0/Q4_1/IQ4_NL/Q5_0/Q5_1 K/V types; the ROCmFPX fork additionally exposes ROCmFP4/FPX and Turbo3/4 types [S33-01, S33-02].

**[RECOMMENDATION]** Persist typed rank-local state with exact layout descriptors. Never treat a byte count or `.bin` blob as sufficient compatibility identity.

## Research split

- Completed now: formulas, upstream/fork type lists, current KV tensor view, shift constraints, FlashAttention/TurboQuant source boundaries.
- Machine work: actual buffer sizes/layouts, FA dispatch/fallback, cache-type quality/speed, shift/restore, long-context and distributed ownership on both nodes.
- Contingent: default K/V types, asymmetric TurboQuant, boundary protection, per-layer sharding and persistent-cache format.

