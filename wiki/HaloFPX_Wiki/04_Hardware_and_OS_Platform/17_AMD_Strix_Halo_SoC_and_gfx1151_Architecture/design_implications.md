---
section_id: "17"
title: "Strix Halo design implications"
status: "needs-machine-validation"
last_verified: "2026-07-16"
applies_to:
  repositories: []
  software_versions: ["ROCm 7.14.0", "Mesa main 20f4f9f45057559475600b60364b60643011990f"]
  hardware_revisions: ["AMD Ryzen AI Max+ 395 / Radeon 8060S; exact project machines OPEN"]
related_sections: ["19", "20", "22", "23", "24", "25", "37", "38", "47", "74"]
---

# Design implications

## Architecture-to-engine mapping

| Evidence | Implication | Proposed rule | Gate |
|---|---|---|---|
| Shared 256-bit LPDDR5X; 40-CU integrated GPU | **[INFERENCE]** CPU preprocessing, GPU inference, media, I/O DMA and a remote transport can contend for the same memory subsystem. | **[RECOMMENDATION]** Budget memory capacity and bandwidth jointly; avoid treating “VRAM” and system RAM as independent pools. | S17-EXP-002 and section 19 |
| `gfx1151`, wave32/wave64, 128 KiB LDS, finite caches/registers | **[INFERENCE]** Tile shape, workgroup size, wave mode, register pressure and fusion determine whether nominal matrix capacity is usable. | **[RECOMMENDATION]** Build target-specific kernels and retain a scalar/non-WMMA correctness fallback. | S17-EXP-003 and section 37 |
| 16C/32T Zen 5 and unknown complex topology | **[INFERENCE]** CPU worker placement can affect shared-L3 traffic and OS/transport latency. | **[RECOMMENDATION]** Discover topology, reserve housekeeping/transport capacity, then benchmark affinity policies. | S17-EXP-001 |
| 45–120 W cTDP and shared package | **[INFERENCE]** short peak clocks cannot select a sustained serving plan. | **[RECOMMENDATION]** compare configurations only after thermal steady state and record actual policy. | S17-EXP-005 and section 22 |
| Two native USB4 controllers | **[INFERENCE]** two physical links may be independent, but the SoC specification alone does not prove this. | **[RECOMMENDATION]** maintain single-link and single-node fallbacks; enable striping only after topology and simultaneous-load evidence. | section 20 |
| ROCm and RADV both have documented paths | **[INFERENCE]** backend choice is a versioned empirical decision, not an architecture constant. | **[RECOMMENDATION]** keep HIP and Vulkan build/test profiles; choose per exact model/kernel/topology. | S17-EXP-003/004, sections 23/25/47/74 |

## Memory-first planning

**[RECOMMENDATION]** A plan manifest should record at least: physical RAM; firmware carveout; HSA coarse/fine-grained pool sizes; Vulkan heap/budget; model weights; KV/state; graph/workspace; transport staging; filesystem cache; and an OS safety reserve. Reject plans whose worst-case allocation exceeds the measured backend-visible budget.

**[INFERENCE]** The 256 GB/s arithmetic ceiling from LPDDR5X-8000 is useful only as a normalization denominator. HaloFPX must measure read, write, copy, and inference traffic separately; a token/s result alone cannot identify bandwidth saturation.

## Kernel and datatype policy

1. **[RECOMMENDATION]** Compile native `gfx1151` code objects and log compiler version, target features, and generated code-object metadata.
2. **[RECOMMENDATION]** Gate WMMA/quantized fast paths with a compile test, deterministic numerical test, and matched performance test. A library accepting the target string is insufficient.
3. **[RECOMMENDATION]** Preserve generic FP32/FP16 or known-correct backend fallbacks. A failed fast-path probe must disable that path, not silently accept corrupt output.
4. **[OPEN]** Select wave32 versus wave64, tile sizes, flash-attention implementation, and quantized kernels only after compiler resource and profiler evidence.

## Distributed implications

**[INFERENCE]** Replication duplicates weights but avoids per-token cross-node collectives; tensor parallelism reduces per-node weight capacity but adds fabric traffic; pipeline parallelism trades bubbles for less frequent boundary transfers; remote speculation adds draft/verification traffic. The SoC facts alone do not select among them.

**[RECOMMENDATION]** Every distributed plan must state rank ownership, which allocations are rank-local, failure behavior, and a single-node fallback. Topology selection must consume measured local memory bandwidth, backend kernel rates, and simultaneous dual-link transport results rather than marketing link rates.

## NPU and media policy

**[RECOMMENDATION]** Keep NPU and VCN use optional and isolated behind adapters. Admit them only when they reduce end-to-end latency or CPU/GPU contention under matched tests; nominal TOPS or codec throughput is not sufficient.

## Contingent decisions

- **[OPEN]** Preferred inference backend: HIP, Vulkan, or per-kernel hybrid.
- **[OPEN]** Preferred power profile and concurrency target.
- **[OPEN]** CPU affinity/isolation policy.
- **[OPEN]** GPU-visible memory reserve and maximum model/KV budget.
- **[OPEN]** WMMA and low-precision paths approved for production correctness.
