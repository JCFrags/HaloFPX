---
section_id: "37"
title: "gfx1151 HIP and Vulkan Kernel Optimization Opportunities"
status: "needs-machine-validation"
last_verified: "2026-07-16"
applies_to:
  repositories: ["charlie12345/ROCmFPX@a5605a72768c6562241b248e268e33dc92787394", "ggml-org/llama.cpp@788e07dc91d266ad3162a1ce9037665656269689"]
  software_versions: []
  hardware_revisions: ["gfx1151 / Strix Halo; exact machine BOM unresolved"]
related_sections: ["24", "25", "27", "28", "33", "73", "74"]
---

# 37 - gfx1151 kernel optimization map

This section is a measurement-driven backlog, not a claim that every listed fusion will win. Decode and prompt fill exercise different shapes; a microkernel gain is promotable only after an end-to-end guard.

- **[VERIFIED]** ROCmFPX commit `a5605a7` contains custom ROCmFP4 HIP code, Vulkan quant/dequant shaders, gfx1151 build guidance, and environment-scoped experiment logs [S37-01].
- **[VERIFIED]** Current llama.cpp has distinct HIP/CUDA-derived and Vulkan backend implementations with graph scheduling, quantized matmul, attention, normalization, and elementwise kernels [S37-02].
- **[VERIFIED]** Vulkan subgroup size is implementation-dependent and must be queried; subgroup size control is exposed only when supported [S37-04].
- **[RECOMMENDATION]** Prioritize measured decode hotspots first: quantized GEMV/MMID/MoE, launch/graph overhead, attention, and small reductions. Then optimize prompt-fill GEMM/attention.
- **[OPEN]** No HaloFPX measurement has yet established the actual hotspot order.

