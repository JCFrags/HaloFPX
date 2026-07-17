---
section_id: "54"
title: "Buffer-path design implications"
status: "needs-machine-validation"
last_verified: "2026-07-16"
applies_to:
  repositories: ["torvalds/linux@8cdeaa50eae8dad34885515f62559ee83e7e8dda"]
  software_versions: ["Linux 7.2-rc2 USB4STREAM", "HIP", "Vulkan"]
  hardware_revisions: ["Radeon 8060S/gfx1151"]
related_sections: ["49", "50", "53", "55", "75", "80"]
---

# Design implications

- **[RECOMMENDATION]** Start with reusable aligned CPU staging buffers and identical framing across TCP and USB4STREAM. Optimize one boundary at a time so copy counts and correctness remain attributable.
- **[RECOMMENDATION]** Represent buffer ownership independently of backend. The scheduler sees a buffer token and completion; it must not depend on dma-buf FDs, `tbstreamX`, HIP pointers, or Vulkan memory handles.
- **[RECOMMENDATION]** Keep control metadata in small coherent CPU-visible memory and bulk payload in a measured coarse/noncoherent path. Never poll a bulk payload as a readiness flag.
- **[INFERENCE]** On Strix Halo, HIP mapped host memory may avoid a physical GPU-to-host copy because CPU and GPU share LPDDR, yet cache policy and GPU access pattern can still make it slower than a device-oriented allocation. Only matched measurements can choose it.
- **[RECOMMENDATION]** Do not add dma-buf or registered-buffer kernel work until Section 55 attributes a material real-workload cost to the current copies and proves upstream USB4STREAM correctness/rollback first.
- **[RECOMMENDATION]** Bound and account pinned/registered memory per rank and rail. Registration failure or memory pressure must fall back to ordinary staging without corrupting the session.
- **[RECOMMENDATION]** Instrument logical bytes, userspace copies, kernel iterator-copy bytes, DMA bytes if traceable, cache operations, faults, syscalls, CPU cycles, GPU idle time, and end-to-end latency separately. Do not infer a copy count from throughput.

## Research split

1. Completed now: API/source semantics and current USB4STREAM copy boundary.
2. On-machine: S54-E01 capability capture, E02 CPU baseline, E03 HIP matrix, E04 Vulkan matrix, E05 io_uring fixed-buffer test, E06 GPU-to-peer-GPU correctness/performance.
3. Contingent decisions: allocator, coherence policy, pool sizing, io_uring use, dma-buf feasibility, and any kernel registered-buffer proposal.
