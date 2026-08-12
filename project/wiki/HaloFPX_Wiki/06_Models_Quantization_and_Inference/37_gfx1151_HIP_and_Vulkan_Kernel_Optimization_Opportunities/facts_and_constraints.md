---
section_id: "37"
title: "gfx1151 kernel facts and constraints"
status: "needs-machine-validation"
last_verified: "2026-07-17"
applies_to:
  repositories: ["charlie12345/ROCmFPX@a5605a72768c6562241b248e268e33dc92787394", "ggml-org/llama.cpp@788e07dc91d266ad3162a1ce9037665656269689"]
  software_versions: []
  hardware_revisions: ["gfx1151"]
related_sections: ["24", "25", "27"]
---

# Facts and constraints

## Architecture and tool facts

**[VERIFIED]** AMD document 70649 describes the RDNA3.5 instruction set and shader-visible state [S37-03]. It is the ISA authority, but instruction availability does not prove compiler emission or application speedup.

**[VERIFIED]** Vulkan compute subgroup properties and supported operations are queried from the physical device; subgroup size can vary, and required-size control depends on the feature/extension [S37-04]. Hard-coding a wave/subgroup width without a device guard is unsafe.

**[VERIFIED]** ROCprofiler-SDK supports HIP/runtime activity tracing, kernel dispatch and memory-copy tracing, and hardware counter collection [S37-05]. Vulkan requires its own timestamp/query and external profiler path; HIP counter names must be discovered on the actual device.

## Current code surface

**[VERIFIED]** ROCmFPX's `ggml/rocmfp4/rocmfp4_hip.cu` and related backend code implement custom quantization/dequantization and matrix-vector/matrix paths; its repository logs test multiple launch geometries and records both promoted and rejected candidates [S37-01]. Those measurements are source-repository evidence only, not `[MEASURED]` HaloFPX results.

**[VERIFIED]** Upstream Vulkan ships format-specific dequant shaders and quantized matmul paths, while graph/backend scheduling is separate [S37-02]. Thus a backend comparison must match numerical format, graph, offload, batch, and memory policy.

## Shape hypotheses requiring profiling

**[ASSUMPTION]** Decode, prompt fill, and MTP verification are expected to expose different operation shapes and reuse opportunities. No HaloFPX profile currently proves which operation class dominates device or end-to-end time. Treat narrow GEMV, larger GEMM, attention, MoE, launch, synchronization, and memory behavior as competing hypotheses until M37-01 links target-machine traces and environment metadata.

**[OPEN]** Whether one kernel geometry is competitive across these phases, and which surface deserves first optimization effort, remains unresolved.
