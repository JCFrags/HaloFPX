---
section_id: "25"
title: "Vulkan and RADV Facts and Constraints"
status: "needs-machine-validation"
last_verified: "2026-07-16"
applies_to:
  repositories:
    - "Mesa 20f4f9f45057559475600b60364b60643011990f"
    - "ggml-org/llama.cpp 788e07dc91d266ad3162a1ce9037665656269689"
  software_versions:
    - "Vulkan 1.4.357"
  hardware_revisions: []
related_sections: ["19", "23", "24", "37", "47", "54"]
---

# Facts and constraints

## Memory properties

| Topic | Evidence-backed statement | HaloFPX constraint |
|---|---|---|
| Heaps and types | **[VERIFIED]** A memory type names a property set and a heap index. A resource's `memoryTypeBits` restricts compatible types; allocation selection must use the queried properties rather than a fixed index. [S25-01] | Persist property flags and heap identity in experiment metadata; never assume the two machines enumerate identical indices. |
| `HOST_VISIBLE` | **[VERIFIED]** Memory with this bit can be mapped with `vkMapMemory`. [S25-01] | Mapping alone gives no ordering or completion guarantee. |
| `HOST_COHERENT` | **[VERIFIED]** Host flush/invalidate calls are unnecessary for host-domain availability/visibility, but queue/host synchronization and device dependencies still apply. [S25-01] | A GPU-produced transport buffer needs a device-to-host dependency plus completion notification before the CPU or network stack reads it. |
| `HOST_CACHED` | **[VERIFIED]** The bit states that host access is cached; the specification notes uncached host memory is slower for host access and is always host coherent. [S25-01] | Prefer cached memory for CPU reads only after measuring GPU and transport effects; use write-combined/non-cached types only for measured one-way writes. |
| Non-coherent memory | **[VERIFIED]** Flush/invalidate ranges must respect `nonCoherentAtomSize`; current RADV main reports 64 bytes in its generic physical-device properties, but the running device must be queried. [S25-01] [S25-03] | Correctness tests must include unaligned logical payloads and aligned API ranges. |
| UMA | **[VERIFIED]** The Vulkan Guide describes `DEVICE_LOCAL | HOST_VISIBLE` as a common UMA combination that can avoid staging. [S25-02] | **[OPEN]** This does not establish zero-copy speed, CPU cache behavior, or the actual Strix Halo heap layout. |

## RADV snapshot

**[VERIFIED]** Mesa commit `20f4f9f45057559475600b60364b60643011990f` maps `CHIP_STRIX_HALO` to `gfx1151`. RADV is a user-mode Vulkan driver over the `amdgpu` kernel driver; ACO is its recommended/default shader compiler in current Mesa documentation. [S25-03] [S25-04]

The following are source-code capabilities, not measurements of either HaloFPX machine:

| Capability | RADV main evidence | Qualification |
|---|---|---|
| Memory types | **[VERIFIED]** The source constructs GTT host-visible/coherent types, cached GTT variants, and visible-VRAM device-local/host-visible/coherent types when corresponding heaps exist. [S25-03] | Heap presence and size depend on kernel-reported topology and configuration. |
| Timeline semaphore | **[VERIFIED]** Feature and extension exposure are gated by `has_timeline_syncobj`. [S25-03] | Must query the running kernel/driver; do not infer from Mesa version alone. |
| Buffer device address | **[VERIFIED]** Advertised; capture-replay is true and multi-device is false in this snapshot. [S25-03] | An address is not a cross-process or cross-host transport identifier. |
| Synchronization2 | **[VERIFIED]** Advertised in the feature table. [S25-03] | Validation still depends on correct stage/access masks. |
| External memory | **[VERIFIED]** `VK_KHR_external_memory_fd` is enabled; `VK_EXT_external_memory_host` is conditional on `has_userptr`. [S25-03] | Query handle-type-specific import/export properties and dedicated-allocation requirements. |
| External semaphore FD | **[VERIFIED]** `VK_KHR_external_semaphore_fd` is enabled in this source snapshot. [S25-03] | Handle compatibility, permanence, and timeline compatibility are per handle type. |
| Subgroups | **[VERIFIED]** RADV reports subgroup operations including arithmetic, ballot, shuffle, clustered, and rotate; for GFX10+ it reports min subgroup size 32 and max 64 with required subgroup size for compute. [S25-03] | Measure wave32 and wave64 per kernel; do not assume one universally wins. |
| Descriptors | **[VERIFIED]** Descriptor indexing and descriptor buffer are exposed in this snapshot. [S25-03] | Availability does not prove lower overhead for ggml's workload. |

## External memory is not cross-host memory

**[VERIFIED]** Vulkan external-memory and external-semaphore extensions export/import platform handles such as POSIX file descriptors. The specification describes opaque FDs as transferable over a Unix socket using `SCM_RIGHTS`; compatibility can require matching driver and device UUIDs. [S25-01] [S25-02]

**[INFERENCE]** An FD cannot be passed by `SCM_RIGHTS` between separate Linux kernels over the project's USB4 IP links. Therefore, external memory may connect Vulkan to another API or process on one rank, but does not remove serialization, framing, transport, integrity, and peer-allocation work between ranks. Cross-rank protocol ownership belongs to sections 49–54.

## Synchronization model

- **[VERIFIED]** Vulkan gives few implicit execution-order guarantees. Correct dependencies require an execution dependency plus appropriate availability and visibility operations. [S25-01]
- **[VERIFIED]** Timeline semaphores carry monotonically increasing payload values and support queue and host waits/signals; they are suitable for tracking completion epochs without allocating one binary semaphore per epoch. [S25-01] [S25-02]
- **[VERIFIED]** A semaphore dependency can carry memory availability/visibility between queue submissions, but stage masks still bound execution scopes. Host access additionally requires observing completion through a fence, semaphore host wait, or another defined mechanism. [S25-01]
- **[RECOMMENDATION]** Define a buffer state machine: `CPU/network writable -> transport received -> GPU readable -> GPU produced -> CPU/network readable -> recycled`. Give each transition an explicit owner, semaphore/fence value, stage mask, access mask, and failure timeout.

## Command, descriptor, and pipeline behavior

- **[VERIFIED]** Vulkan command buffers can be re-submitted after leaving pending state unless recorded one-time; pending buffers cannot be modified. Queue submission may be high-overhead, so the specification advises batching submissions. [S25-01]
- **[VERIFIED]** The Khronos sample finds reset/reuse preferable to allocate/free in its tested graphics workload, but those sample timings are not HaloFPX measurements. [S25-02]
- **[VERIFIED]** Descriptor-set updates are host operations with synchronization rules. Khronos documents allocation/update churn as a possible CPU overhead. [S25-01] [S25-02]
- **[VERIFIED]** A `VkPipelineCache` can be serialized and reused only when its header is compatible with vendor ID, device ID, and `pipelineCacheUUID`; incompatible data is ignored. [S25-01]
- **[VERIFIED]** RADV translates SPIR-V to NIR and then ACO ISA. Mesa also has an on-disk shader cache controlled by documented `MESA_SHADER_CACHE_*` variables. [S25-04] [S25-05]

## llama.cpp Vulkan snapshot

At commit `788e07dc91d266ad3162a1ce9037665656269689`:

- **[VERIFIED]** device initialization reads Vulkan 1.2 buffer-device-address support; compute dispatches use buffer descriptors and push constants. [S25-06]
- **[VERIFIED]** descriptor sets are allocated in pools and a storage-buffer descriptor set is updated for each recorded dispatch. [S25-06]
- **[VERIFIED]** command pools use transient and per-command-buffer reset flags, and the backend retains command buffers in a pool. [S25-06]
- **[VERIFIED]** asynchronous transfer/compute coordination uses timeline-semaphore submission data when the separate transfer queue path is active. [S25-06]
- **[VERIFIED]** compute pipeline creation passes `VK_NULL_HANDLE` as the application pipeline cache. Mesa's internal disk cache may still help, but llama.cpp does not persist its own `VkPipelineCache` in this snapshot. [S25-05] [S25-06]

These implementation facts are volatile and must be re-audited when the project freezes a llama.cpp/ROCmFPX baseline.

