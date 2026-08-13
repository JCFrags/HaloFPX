---
section_id: "30"
title: "ROCmFPX Weight Formats and Quantization Recipes"
status: "needs-machine-validation"
last_verified: "2026-08-12"
applies_to:
  repositories: ["charlie12345/ROCmFPX", "JCFrags/HaloFPX"]
  software_versions: ["a5605a72768c6562241b248e268e33dc92787394", "4a156395db62604cf37e27e6459e3ee0e3949c48", "6c88472bf5f567a1064f27f4d8a90fc8e2b47a02", "b77f2bce6e7875ab065e09894f45915585c9f156"]
  hardware_revisions: ["Strix Halo gfx1151 - local validation pending"]
related_sections: ["29", "31", "33", "37"]
---

# ROCmFPX formats and recipes

**[VERIFIED]** The pinned fork defines custom weight tensor types numbered 100-104 and 107, plus TurboQuant KV types 105-106 [S30-01]. ROCmFP4/FPX weight formats and TurboQuant cache formats are different contracts even though the fork exposes both through runtime cache-type parsing.

**[VERIFIED current-source reconciliation]** HaloFPX `4a156395` retains Q2,
Q3, Q4 dual-scale, Q4 FAST, Q6, and Q8 weight formats. Q2 uses the S40
codebook `-4,-1,+1,+4`, is direct-quantizer-only, is absent from the common
application K/V-cache CLI allowlist, and has CPU plus partial shared CUDA/HIP
static wiring but no Vulkan path. Q3, Q4, Q4_FAST, Q6, and Q8 have
CPU/CUDA/HIP/Vulkan paths. Q6's 26-byte serialized
block covers `[-32,31]` and expands to 34 bytes in the Vulkan device layout
[S30-L01]. Static support is not a model, quality, or performance claim.

**[RECOMMENDATION]** Store a recipe as an architecture-qualified policy ID plus the resolved per-tensor type map. Names such as `AGENT`, `COHERENT`, and `STRIX_LEAN` are policy labels, not universal quality guarantees.

## Research split

- Completed now: exact identifiers, block layouts, nominal BPW, preset routing source, and documented CPU/CUDA/HIP/Vulkan implementation claims at commit `a5605a7`.
- Machine work: compile gfx1151, run reference and backend-op gates, quantize each target architecture, inspect actual BPW, compare quality and performance.
- Contingent: production default, protected tensor map, whether any ROCmFPX type is safe for K/V, and per-architecture recipe versions.
