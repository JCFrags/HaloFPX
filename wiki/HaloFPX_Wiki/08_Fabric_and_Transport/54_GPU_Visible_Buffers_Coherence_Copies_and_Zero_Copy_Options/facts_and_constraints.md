---
section_id: "54"
title: "Buffer-path facts and constraints"
status: "needs-machine-validation"
last_verified: "2026-07-16"
applies_to:
  repositories: ["torvalds/linux@8cdeaa50eae8dad34885515f62559ee83e7e8dda"]
  software_versions: ["Linux 7.2-rc2 USB4STREAM", "HIP 7.2.53211 documentation", "Vulkan 1.4.356"]
  hardware_revisions: ["Radeon 8060S/gfx1151"]
related_sections: ["19", "24", "25", "50", "53"]
---

# Facts and constraints

## Candidate paths

| Path | What it can remove | What remains / correctness requirement |
|---|---|---|
| Pageable CPU staging | Nothing; portable baseline | GPU↔CPU transfer/access, syscall, kernel copy, peer staging |
| HIP `hipHostMallocMapped` | Separate device allocation/copy may be avoidable | GPU completion and host/device coherence; USB4 driver still copies |
| HIP coherent pinned | GPU and CPU can share pointer with fine-grained semantics | Potential cache/bandwidth cost; pinned-memory pressure; driver copy |
| HIP noncoherent pinned | May improve bulk GPU access | Explicit synchronization before CPU/transport use; driver copy |
| Vulkan HOST_VISIBLE + HOST_COHERENT | Host mapping without explicit flush/invalidate for host coherence | Queue ownership/barriers/fence; driver copy |
| Vulkan HOST_VISIBLE noncoherent | Wider memory-type choice | Atom-aligned flush/invalidate plus queue/barrier/fence |
| io_uring fixed buffers | Repeated page validation/pinning/mapping overhead | Does not promise payload zero-copy; underlying file operation may copy |
| dma-buf | Cross-driver buffer identity and fence sharing | Requires exporter and USB4 importer support, DMA mapping, lifetime and CPU-access synchronization |
| New registered-buffer USB4 ABI | Could allow driver DMA against reusable pages | Kernel patch, long-term pin accounting, security, invalidation, DMA/IOMMU, completion/lifetime ABI |

- **[VERIFIED]** `hipHostMallocMapped` maps a pinned allocation into the current device address space; the device pointer is obtained with `hipHostGetDevicePointer` [S54-01]. Coherent and noncoherent flags have different cache/synchronization implications [S54-02].
- **[VERIFIED]** Vulkan memory visibility and execution completion are distinct. Host visibility/coherence flags do not replace pipeline barriers, semaphores, or fences needed to transfer ownership between GPU work and the host [S54-03].
- **[VERIFIED]** dma-buf CPU access must be bracketed by synchronization; implicit fences can signal DMA completion, but explicit synchronization users remain responsible for their own ordering [S54-04].
- **[VERIFIED]** io_uring registered buffers pin/map reusable anonymous memory to reduce per-I/O overhead. Registration is not itself a no-copy guarantee [S54-05].
- **[VERIFIED]** USB4STREAM v7.2-rc2 frames payloads in at most 4 KiB and uses `copy_page_from_iter()`/`copy_page_to_iter()` around page-backed DMA rings [S54-06]. Large `writev` can reduce syscalls but not these copies.

## Required ownership state machine

```text
FREE -> GPU_WRITING -> GPU_COMPLETE -> TRANSPORT_READING
     -> WIRE/PEER -> TRANSPORT_WRITING -> CPU/GPU_VISIBLE
     -> PEER_GPU_READING -> COMPLETE -> FREE
```

**[RECOMMENDATION]** Every transition needs an explicit completion primitive, generation/epoch, buffer ID, valid byte range, and single owner. Cancellation must not recycle a buffer until GPU and kernel operations are proven complete.

## Hazards

Early reuse, stale cache lines, missing Vulkan flush/invalidate, treating HIP events as system visibility without verifying flags, dma-buf fence misuse, long-term pin exhaustion, unbounded registered pools, use-after-unregister, partial read/write handling, aliasing across epochs, and claiming “zero-copy” from syscall-count reduction alone.
