---
section_id: "24"
title: "HIP, HSA, RCCL facts and constraints"
status: "needs-machine-validation"
last_verified: "2026-07-16"
applies_to:
  repositories:
    - "ROCm/HIP@bc9af25177f96c0fea93198b89cf4c3cf08f3ea3"
    - "ROCm/ROCR-Runtime@e5498ba92dad7099d2027bd22bd7295ca1caf833"
    - "ROCm/rccl@96a25b5fd6f73fba58c7d83eb57cf19a50230aa4"
  software_versions: ["ROCm 7.2.3", "HIP 7.2.53211 documentation", "RCCL 2.27.7"]
  hardware_revisions: ["AMD Strix Halo gfx1151; machine properties unmeasured"]
related_sections: ["19", "23", "27", "54"]
---

# Facts and constraints

## Platform and allocation model

| Topic | Evidence-backed conclusion | HaloFPX constraint |
|---|---|---|
| gfx1151 support | **[VERIFIED]** ROCm 7.2's Ryzen Linux matrix lists gfx1151, including Ryzen AI Max+ 395/Max 390/Max 385 [S24-002]. | Exact host model, supported OS image, firmware, and package set still require EX24-01. |
| Strix Halo memory | **[VERIFIED]** AMD describes Strix Halo GPU memory access as GPUVM-backed per-process address spaces, not a separate discrete VRAM pool [S24-001]. | Unified physical DRAM does not imply every allocation is coherent, equally cached, pinned, or zero-copy efficient. |
| HIP allocation classes | **[VERIFIED]** HIP exposes device allocation (`hipMalloc`), pinned host allocation (`hipHostMalloc`), managed allocation (`hipMallocManaged`), and system/pageable allocation paths [S24-003]. | Record the actual pointer attributes and memory-pool backing; API names alone do not prove placement. |
| Mapped pinned host memory | **[VERIFIED]** `hipHostMallocMapped` maps host allocation into the current device address space and `hipHostGetDevicePointer` obtains its device pointer [S24-004]. | Mapping proves addressability, not required bandwidth, atomicity, or remote-host visibility. |
| Fine/coarse allocation | **[VERIFIED]** HIP documents fine-grained host/managed paths and coarse/fine control via flags or advice; ROCR exposes pool flags and access properties [S24-003, S24-007]. | Query both HIP device attributes and HSA pools on each machine. |

## Coherence and ordering

**[VERIFIED]** HIP's current coherence page says `hipHostMallocMapped` is fine-grained and ignores `hipHostMallocNonCoherent`. It also documents `hipHostMallocCoherent`, `hipHostMallocNonCoherent`, `hipDeviceMallocFinegrained`, and `hipMemAdviseSetCoarseGrain` controls [S24-003].

**[OPEN]** The same official coherence page is internally inconsistent about an unflagged `hipHostMalloc`: its table labels `hipHostMallocDefault` fine-grained while its footnote says an unset/zero `HIP_HOST_COHERENT` with no explicit coherent/mapped flag yields coarse-grained memory [S24-003]. Do not rely on a default; request an explicit mode and run EX24-03.

**[VERIFIED]** HIP states that memory accesses are not automatically observed in program order across threads. `__threadfence_block`, `__threadfence`, and `__threadfence_system` order at block, device, and system scope; `_system` atomic variants operate at system scope subject to hardware support [S24-005].

**[VERIFIED]** HIP exposes `hipDeviceAttributeFineGrainSupport` and `hipDeviceAttributeHostNativeAtomicSupported`; the 7.2.3 header describes the latter as native atomics over the host-device link [S24-006]. **[RECOMMENDATION]** Gate the system-atomic protocol on returned attributes plus a litmus test, never on `gfx1151` alone.

**[VERIFIED]** HIP documents system-scope release behavior for `hipStreamSynchronize` and `hipDeviceSynchronize`, while `hipStreamWaitEvent` creates a stream dependency without a host-facing fence [S24-003]. **[OPEN]** The same HIP 7.2.53211 coherence page is inconsistent for events: its table labels `hipEventSynchronize` a system-scope release, but its following text says a default recorded event uses device scope and that `hipEventReleaseToSystem` requests the stronger system release [S24-003]. `hipEventDisableSystemFence` explicitly removes system fencing and may leave device memory invisible to host or other devices [S24-008]. Use an explicit release mode and EX24-04.

**[VERIFIED]** HSA AQL packet headers carry acquire and release fence scopes (`none`, `agent`, `system`), and the barrier bit delays a packet until preceding packets in that queue complete [S24-007]. Fine-grained pools support shared visibility; coarse-grained pool writes are restricted to one agent at a time [S24-007].

## Streams, events, graphs, and queues

- **[VERIFIED]** Operations in one HIP stream are ordered; independent streams require explicit dependencies such as record-event/wait-event [S24-009].
- **[VERIFIED]** A host `hipStreamSynchronize` blocks until that stream's submitted commands complete. A null-stream synchronize follows null-stream semantics and can wait on other same-device streams [S24-008].
- **[VERIFIED]** Timing-disabled events avoid timing collection overhead [S24-003, S24-008]. When the host or another local agent must see writes, explicitly request `hipEventReleaseToSystem`, never `hipEventDisableSystemFence`, and validate the flag combination on the pinned runtime.
- **[VERIFIED]** HIP graphs encode operations and dependencies; capture records stream operations until `hipStreamEndCapture`. Non-HIP functions are not captured merely because they execute while capture is active [S24-010].
- **[VERIFIED]** ROCm 7.2.1 fixed global-capture validation, event-query/event-synchronize capture restrictions, and an AQL batch-dispatch doorbell CPU-hang issue [S24-011]. Pin at least 7.2.1 behavior when evaluating capture; this section's baseline is 7.2.3.
- **[INFERENCE]** A HIP stream is a software ordering abstraction and must not be treated as permanently one-to-one with a hardware HSA queue. Measure queue IDs through tracing before designing queue-affinity assumptions [S24-012].

## Peer access and two hosts

**[VERIFIED]** `hipDeviceCanAccessPeer` reports whether one locally enumerated HIP device can directly access memory physically located on another locally enumerated device [S24-013].

**[INFERENCE]** The GPU in the second computer is not a local HIP device, so HIP peer access is not the dual-host data path. The USB4 transport must move bytes between address spaces even if each APU locally uses unified DRAM.

## RCCL and network plugins

**[VERIFIED]** RCCL provides multi-GPU/multi-node collectives. It can dynamically load `librccl-net.so` or a named `librccl-net-${NCCL_NET_PLUGIN}.so` and discovers versioned `ncclNet_vX` interfaces [S24-014].

**[VERIFIED]** RCCL 7.2.3's plugin header advertises host, device (`NCCL_PTR_CUDA`, retained NCCL naming), and DMA-BUF pointer capabilities; the implementation must report which it supports [S24-015]. **[OPEN]** No source proves that an existing plugin can directly consume gfx1151 buffers over the project's dual USB4 links.

**[VERIFIED]** RCCL exposes `NCCL_SOCKET_IFNAME` for selecting socket interfaces and debug/algorithm controls [S24-016]. **[RECOMMENDATION]** Use socket RCCL only as a measured baseline. A plugin decision depends on two-rank latency, message-size crossover, registration cost, and failure behavior.

## Profiling

**[VERIFIED]** `rocprofv3` can separately trace HIP APIs, HSA APIs, kernels, memory copies, allocations, scratch, queues/KFD, and RCCL. Memory-copy records include stream and source/destination agent IDs; kernel records include queue IDs [S24-012].

**[RECOMMENDATION]** Capture traces for correctness experiments before enabling counters. Counter collection can perturb timing and belongs primarily to section 27.
