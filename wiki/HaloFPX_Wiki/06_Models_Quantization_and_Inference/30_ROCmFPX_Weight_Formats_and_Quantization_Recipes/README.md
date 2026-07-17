---
section_id: "30"
title: "ROCmFPX Weight Formats and Quantization Recipes"
status: "needs-machine-validation"
last_verified: "2026-07-16"
applies_to:
  repositories: ["charlie12345/ROCmFPX"]
  software_versions: ["a5605a72768c6562241b248e268e33dc92787394"]
  hardware_revisions: ["Strix Halo gfx1151 - local validation pending"]
related_sections: ["29", "31", "33", "37"]
---

# ROCmFPX formats and recipes

**[VERIFIED]** The pinned fork defines custom weight tensor types numbered 100-104 and 107, plus TurboQuant KV types 105-106 [S30-01]. ROCmFP4/FPX weight formats and TurboQuant cache formats are different contracts even though the fork exposes both through runtime cache-type parsing.

**[RECOMMENDATION]** Store a recipe as an architecture-qualified policy ID plus the resolved per-tensor type map. Names such as `AGENT`, `COHERENT`, and `STRIX_LEAN` are policy labels, not universal quality guarantees.

## Research split

- Completed now: exact identifiers, block layouts, nominal BPW, preset routing source, and documented CPU/HIP/Vulkan implementation claims at commit `a5605a7`.
- Machine work: compile gfx1151, run reference and backend-op gates, quantize each target architecture, inspect actual BPW, compare quality and performance.
- Contingent: production default, protected tensor map, whether any ROCmFPX type is safe for K/V, and per-architecture recipe versions.

