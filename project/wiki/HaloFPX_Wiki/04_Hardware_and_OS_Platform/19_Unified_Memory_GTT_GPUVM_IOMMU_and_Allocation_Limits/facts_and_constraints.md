---
section_id: "19"
title: "Unified-memory facts and constraints"
status: "needs-machine-validation"
last_verified: "2026-08-12"
applies_to:
  repositories: []
  software_versions: ["ROCm 7.14 documentation", "Linux amdgpu/TTM"]
  hardware_revisions: ["Radeon 8060S / gfx1151"]
related_sections: ["17", "23", "24", "54"]
---

# Facts and constraints

## Layers that must not be conflated

| Layer | Meaning | Limit type |
|---|---|---|
| Physical LPDDR | CPU/GPU backing store shared by the APU | Physical capacity and bandwidth |
| BIOS VRAM carve-out | RAM reserved early and presented as dedicated/visible VRAM | Firmware policy; reduces ordinary host RAM |
| TTM/GTT domain | System-memory pages available for GPU buffer placement/mapping | Kernel policy/accounting, page based |
| GPUVM | Per-process GPU virtual address mappings to VRAM/system/MMIO pages | Address-space/mapping policy |
| HIP allocation | `hipMalloc`, managed, pinned-host, registered/system allocations | Runtime/API semantics and capability |
| IOMMU | Translation/isolation for DMA-capable devices | Platform security and mapping behavior |

- **[VERIFIED]** On APUs CPU and GPU can share the same physical memory [S19-05]. Shared backing does not erase the software domains above.
- **[VERIFIED]** TTM buffer objects can move between resource domains and have CPU mappings; therefore reported “VRAM,” GTT, resident memory, and process RSS are different views and must be sampled together [S19-03].
- **[VERIFIED]** HIP describes `hipHostMalloc` as pinned host memory, `hipMallocManaged` as managed memory, and `hipMalloc` as device allocation. Supported migration/coherence behavior depends on device/system capability attributes [S19-06].
- **[VERIFIED]** Pinned allocations do not automatically migrate; managed/HMM behavior can fault and migrate pages when supported. Fine-grained coherence is useful for synchronization but can cost bandwidth versus coarse-grained bulk data [S19-06, S19-07].
- **[VERIFIED]** `amdgpu.no_system_mem_limit` disables a system-memory reservation limit for multiple-process shared memory; it does not add physical RAM [S19-08].
- **[INFERENCE]** A large GTT aperture is addressability/accounting capacity, not eager physical reservation. Admission must still subtract OS, page tables, pinned transport buffers, model metadata, compute workspaces, KV cache, display, filesystem cache, and failure margin.

## Historical cluster state

**[VERIFIED]** Preserved July 12 audit reports state that each node had about 124 GiB usable host memory, 1 GiB firmware VRAM, a 126,976 MiB GTT aperture/cap, and only tens of MiB GTT use while unloaded [S19-L01, S19-L02]. They record `amdgpu.gttsize=126976`, `ttm.pages_limit=32505856`, `ttm.page_pool_size=32505856`, and `amd_iommu=off`; they also record the deprecated-parameter warning and NPU failure. The raw command-output bundle was unavailable, so S19-E01 must recapture these values before they are treated as `[MEASURED]` current state.

**[OPEN]** These values must be recaptured because kernel/runtime configuration and workloads change them.

## Live cluster state — 2026-07-17

- **[MEASURED]** `nimo-1` reported 130,491,708 KiB total memory and `nimo-2` 130,491,700 KiB; both exposed one NUMA node [S19-L03].
- **[MEASURED]** Both booted with `amdgpu.gttsize=126976`, `ttm.pages_limit=32505856`, `ttm.page_pool_size=32505856`, and `amd_iommu=off`; `rocm-smi` reported 133,143,986,176 bytes total GTT on each host [S19-L03].
- **[MEASURED]** The loaded RPC worker consumed about 76.0 GiB by its systemd cgroup on nimo-1, while the coordinator/model server consumed about 79.6 GiB on nimo-2. These values include more than model tensors and must not be added to `rocm-smi` GTT as independent capacity [S19-L03].
- **[MEASURED]** Both had 32 GiB swap, but nimo-1 used priority `-1` and nimo-2 priority `100`; nimo-2 also explicitly disabled zswap while nimo-1's command line did not carry the same flag [S19-L03].
- **[RECOMMENDATION]** Normalize or intentionally freeze swap/zswap policy before allocation staircases, OOM/fallback drills, or matched inference comparisons.

## Production HMM/global-OOM incident — 2026-08-12

- **[MEASURED]** nimo-2 reported approximately 14 GiB available while the production RPC worker owned `114041696 kB` of `gpu_active` HMM pages. The kernel then invoked global OOM four times and killed the worker after killing smaller user-session processes [S19-L04].
- **[MEASURED]** The active `-j2` build's visible `cmake`, `ninja`, and `cc1plus` residents were small relative to the HMM owner. This does not prove a compiler RSS leak; it proves that ordinary free/available/RSS views failed as a safe concurrent-work admission predicate [S19-L04].
- **[RECOMMENDATION]** Reject target builds, quantization, disposable inference, and benchmarks whenever protected production or unaccounted KFD/render/HMM ownership exists. Require an authorized maintenance window and GPU-owner census; do not let `MemAvailable`, free RAM, swap, or RSS override the ownership gate.

## Failure modes

- Reservation/virtual mapping rejection despite nominal free RAM.
- Physical OOM, reclaim stalls, PSI growth, swap storms, or pinned-memory starvation.
- GPU page faults, queue eviction/reset, or allocation failure under fragmentation/contention.
- Double counting or misleading capacity conclusions from VRAM/GTT/RSS alone.
- IOMMU-off loss of DMA isolation and NPU functionality; IOMMU-on performance/mapping changes.
- Coherence mistakes when CPU, GPU, and transport share buffers.
