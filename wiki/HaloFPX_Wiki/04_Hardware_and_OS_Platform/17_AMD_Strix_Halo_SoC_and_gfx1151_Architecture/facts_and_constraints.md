---
section_id: "17"
title: "Strix Halo facts and constraints"
status: "needs-machine-validation"
last_verified: "2026-07-16"
applies_to:
  repositories: []
  software_versions: ["ROCm 7.14.0", "ROCm 7.1.1", "LLVM main c571b0bd7330a4b737ad7dec31e7f2b52edd3953", "Linux fce2dfa773ced15f27dd27cd0b482a7473cdcf2a"]
  hardware_revisions: ["AMD Ryzen AI Max+ 395 / Radeon 8060S; exact project machines OPEN"]
related_sections: ["18", "19", "20", "22", "23", "24", "25", "37"]
---

# Facts and constraints

## CPU, package, memory, and I/O

| Item | Evidence-backed statement | HaloFPX constraint |
|---|---|---|
| CPU | **[VERIFIED]** 16 Zen 5 cores, 32 threads, 3.0 GHz base, up to 5.1 GHz boost, 16 MiB L2, 64 MiB L3, AVX-512 listed ([S17-SRC-001](sources.md#s17-src-001)). | Boost is a ceiling, not a sustained all-core guarantee. CPU-side tokenization, orchestration, and fallback kernels need measured affinity and frequency data. |
| Complex topology | **[OPEN]** AMD's cited product record says three package dies but does not specify CCD/CCX count, core-to-L3 sharing, or inter-die latency. | Do not encode a presumed 2x8 layout. Discover topology per machine with S17-EXP-001. |
| Memory interface | **[VERIFIED]** 256-bit LPDDR5X, up to 128 GB, up to LPDDR5X-8000 ([S17-SRC-001](sources.md#s17-src-001)). | **[INFERENCE]** The signaling-width product gives 256 GB/s theoretical peak (8000 MT/s x 32 B), before protocol and contention losses. This is not measured usable bandwidth. |
| Power | **[VERIFIED]** 55 W default TDP, 45–120 W cTDP, 100 C Tjmax ([S17-SRC-001](sources.md#s17-src-001)). | CPU, GPU, memory and NPU operate inside platform power/thermal policy; SKU limits do not prove an OEM exposes 120 W. |
| Package | **[VERIFIED]** FP11, three dies; CPU cores and I/O die are listed as TSMC 4 nm ([S17-SRC-001](sources.md#s17-src-001)). | Exact board routing and firmware are outside this page and remain section 18 evidence. |
| I/O | **[VERIFIED]** Two native USB4 40 Gbps ports; three USB 3.2 Gen2; three USB 2.0; PCIe 4.0 with 16 total/usable native lanes ([S17-SRC-001](sources.md#s17-src-001)). | Native controller count does not prove connector independence, lane routing, or simultaneous host-to-host throughput; section 20 must measure it. |

## GPU target and resources

| Item | Evidence-backed statement | Limitation |
|---|---|---|
| Marketing/target identity | **[VERIFIED]** Radeon 8060S has 40 graphics cores and up to 2900 MHz on the product page; ROCm 7.14 maps consumer Max+ 395 to `gfx1151`; pinned LLVM maps Radeon 8060S to `gfx1151` ([S17-SRC-001](sources.md#s17-src-001), [S17-SRC-002](sources.md#s17-src-002), [S17-SRC-004](sources.md#s17-src-004)). | Installed firmware/runtime must report the same target. |
| Generation | **[VERIFIED]** ROCm's GPU table calls Radeon 8060S on Ryzen AI Max+ PRO 395 RDNA 3.5 / GFXIP 11.5 and `gfx1151` ([S17-SRC-003](sources.md#s17-src-003)). | The table names the PRO product; reuse for the consumer part is limited to their shared 8060S identity and must be machine-checked. |
| Execution shape | **[VERIFIED]** The table lists 40 CUs, wave32 or wave64, and 128 KiB LDS per CU ([S17-SRC-003](sources.md#s17-src-003)). | Kernel-selected wave size, occupancy, and usable LDS depend on compiler and kernel. |
| Cache/register hierarchy | **[VERIFIED]** ROCm lists 32 MiB Infinity Cache, 2 MiB L2, 256 KiB graphics L1, 32 KiB L0 vector, 16 KiB L0 scalar, 32 KiB L0 instruction, 768 KiB VGPR and 32 KiB SGPR per CU ([S17-SRC-003](sources.md#s17-src-003)). | Vendor table values do not establish application hit rates or effective capacity. |
| LLVM target properties | **[VERIFIED]** Pinned LLVM identifies `gfx1151` as APU, supports wave64/cumode target features, architected flat scratch and packed work-item IDs, and notes not all VGPRs are usable on `gfx1151` ([S17-SRC-004](sources.md#s17-src-004)). | Compile specifically for `gfx1151`; occupancy calculations must use compiler resource reports, not the nominal VGPR file alone. |
| Linux IP blocks | **[VERIFIED]** Pinned Linux documentation lists Strix Halo as DCN 3.5.1, GC 11.5.1, VCN 4.0.6, SDMA 6.1.1, MP0/MP1 14.0.1 ([S17-SRC-005](sources.md#s17-src-005)). | Documentation describes the family; running kernel/firmware compatibility is section 23. |

## Matrix and data-type capabilities

- **[VERIFIED]** AMD's RDNA 3 WMMA technical article documents 16x16x16 wave matrix operations with FP16 or BF16 inputs and 8-bit or 4-bit integer inputs ([S17-SRC-007](sources.md#s17-src-007)).
- **[VERIFIED]** AMD's ROCm changelog says rocWMMA is built with the `gfx1151` target for ROCm 7.0 and later ([S17-SRC-008](sources.md#s17-src-008)).
- **[INFERENCE]** Because ROCm classifies `gfx1151` as RDNA 3.5 and explicitly builds rocWMMA for it, FP16/BF16/I8/I4 WMMA paths are plausible optimization candidates. These two sources do not prove that every intrinsic, rocWMMA tile, accumulation type, or llama.cpp kernel is supported and correct on a given release.
- **[OPEN]** FP32, FP16, BF16, INT8, INT4, quantized packing, WMMA compilation, numerical correctness, and achieved throughput must each be tested against exact compiler/library commits (S17-EXP-003). FP8 support is not established by the cited `gfx1151` evidence and must not be assumed.

## Media and NPU relevance

- **[VERIFIED]** The product page lists hardware encode for H.264/H.265/AV1 and decode for H.264/VP9/H.265/AV1/MJPEG, with codec/bit-depth-dependent maxima ([S17-SRC-001](sources.md#s17-src-001)).
- **[INFERENCE]** VCN may accelerate multimodal ingest or output, but it does not execute transformer GEMMs; media activity can still contend for power and memory bandwidth.
- **[VERIFIED]** AMD lists an NPU and up to 50 NPU TOPS, with up to 126 aggregate platform TOPS ([S17-SRC-001](sources.md#s17-src-001)).
- **[RECOMMENDATION]** Exclude the NPU from the initial HaloFPX execution plan unless an exact runtime, supported operators/data types, memory-transfer semantics, and reproducible model path are demonstrated. Aggregate TOPS is not a usable LLM throughput metric.

## ROCm and Vulkan support boundaries

- **[VERIFIED]** ROCm 7.14.0 lists Ryzen AI Max+ 395 (`gfx1151`) and a supported OS/driver matrix ([S17-SRC-002](sources.md#s17-src-002)). **[RECOMMENDATION]** Pin the exact matrix release; “ROCm supports gfx1151” does not imply every component or distro combination is supported.
- **[VERIFIED]** Mesa documents RADV support for all graphics-capable RDNA GPUs supported by the Linux kernel, with Vulkan 1.4 for GFX8 and newer ([S17-SRC-006](sources.md#s17-src-006)).
- **[INFERENCE]** A sufficiently new Linux kernel plus matching Mesa should provide a Radeon 8060S Vulkan path, but only `vulkaninfo` and a correctness workload establish the actual device, extensions, heaps, and usable cooperative-matrix path.
