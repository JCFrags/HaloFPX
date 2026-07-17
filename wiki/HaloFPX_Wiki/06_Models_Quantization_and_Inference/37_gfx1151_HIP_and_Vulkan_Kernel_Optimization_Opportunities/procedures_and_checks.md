---
section_id: "37"
title: "gfx1151 profiling and optimization procedure"
status: "needs-machine-validation"
last_verified: "2026-07-16"
applies_to:
  repositories: ["charlie12345/ROCmFPX@a5605a72768c6562241b248e268e33dc92787394", "ggml-org/llama.cpp@788e07dc91d266ad3162a1ce9037665656269689"]
  software_versions: []
  hardware_revisions: ["gfx1151"]
related_sections: ["27", "73", "74", "78"]
---

# Procedures and checks

## M37-01 baseline hotspot census

Prerequisites: exact model/GGUF and binary hashes, clocks/power/thermal policy, no root unless profiler installation requires it.

1. Build HIP for the exact gfx1151 target and Vulkan with shader/debug metadata retained in a separate profiling build.
2. Run matched prompt-fill, single-token decode, MTP `n=2/4/8`, MoE, and concurrent-server shapes.
3. HIP: collect dispatch/memory trace first, then targeted counters with `rocprofv3`; query available counters rather than assuming names [S37-05]. Vulkan: collect timestamp-query spans and driver profiler capture.
4. Attribute wall time to kernels, launches, host gaps, copies, and synchronization. Record grid/block, registers/LDS when available, bytes, occupancy proxy, cache behavior, and graph reuse.
5. Publish raw outputs and environment metadata under `experiments/`, not this wiki page.

## M37-02 candidate loop

For one hotspot at a time: write hypothesis; add capability/shape guard; compare original and candidate in randomized repetitions; test boundary/misaligned shapes; run quant error/logit/output checks; then run end-to-end prompt/decode/MTP/MoE guards. Reject a microbenchmark-only win that regresses a required workload.

## Required matrix

| Dimension | Values |
|---|---|
| backend | HIP, Vulkan |
| operation width | `n=1,2,4,8`, representative prompt batches |
| quant | each supported ROCmFPX format plus one upstream control |
| model | dense, MoE, MTP-capable, hybrid if supported |
| context | short and near target maximum |
| concurrency | 1 and production candidate levels |

## Promotion gate

**[RECOMMENDATION]** Require correctness, no unsupported-device regression, reproducible end-to-end improvement with uncertainty, bounded compile/binary cost, and automatic fallback. Preserve rejected candidates and reasons in experiment records.

