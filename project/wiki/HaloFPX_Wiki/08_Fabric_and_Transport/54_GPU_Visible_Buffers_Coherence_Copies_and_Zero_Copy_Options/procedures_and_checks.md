---
section_id: "54"
title: "Buffer-path procedures and checks"
status: "needs-machine-validation"
last_verified: "2026-07-17"
applies_to:
  repositories: ["torvalds/linux@8cdeaa50eae8dad34885515f62559ee83e7e8dda"]
  software_versions: ["Linux 7.2-rc2 USB4STREAM", "HIP", "Vulkan", "io_uring"]
  hardware_revisions: ["Radeon 8060S/gfx1151"]
related_sections: ["19", "24", "25", "50", "53", "55", "75"]
---

# Procedures and checks

Use exact source/build hashes, idle gates, checksummed deterministic payloads, bounded memory, and the same record codec on every backend. Root is required only for approved tracing/kernel experiments; application tests run unprivileged.

All negative-barrier, pool-exhaustion, contention, cancellation, and rail-failure tests use disposable generated buffers and dedicated test processes under explicit memory/pinned-memory/NVMe/thermal ceilings. Physical rail loss, privileged tracing, and any kernel/device mutation require separate Section 80 authorization, exact resolved targets, preserved out-of-band recovery, stop conditions, cleanup, and a baseline smoke. Never expose a production model/cache store, boot/storage path, workspace, or sole evidence copy.

## S54-E01 — Capability snapshot

Capture kernel/config, page size, IOMMU, memory limits, HIP allocation/coherence attributes, Vulkan heaps/types/noncoherent atom size/external-memory extensions, io_uring probe results, dma-buf heaps, and USB4STREAM file operations from the exact source/build.

## S54-E02 — CPU staging reference

GPU produces deterministic buffers; wait for documented completion; copy/access through ordinary aligned host buffers; send over same-kernel TCP and, only after Section 50 gates, USB4STREAM; peer verifies and feeds GPU. Sweep 64 B–4 MiB, queue depths 1/4/16/64, both directions. Retain every ownership timestamp and digest.

## S54-E03 — HIP mapped/coherence matrix

Compare pageable, `hipHostMallocMapped`, coherent pinned, and noncoherent pinned buffers. For each, test explicit GPU event/stream synchronization and system-visibility primitives required by the installed HIP version. Inject missing-barrier negative tests only in an isolated correctness harness. Measure GPU kernel time, host readiness, transport time, peer GPU consumption, CPU cycles, cache/fault counters, and pinned bytes.

## S54-E04 — Vulkan host-visible matrix

Enumerate actual memory types. Compare HOST_COHERENT and noncoherent types using correct atom-aligned flush/invalidate calls, pipeline barriers, queue submissions, and fences. Hash the full valid range and guard regions. Record memory type/heap and all synchronization operations.

## S54-E05 — io_uring registered-buffer test

Probe supported operations first. Compare `readv/writev` with fixed registered buffers on identical TCP and USB4STREAM files. Verify short I/O, `EAGAIN`, cancellation, unregister-after-completion, pool exhaustion, and fallback. Attribute reduced setup/syscall cost separately from driver iterator-copy cost.

## S54-E06 — End-to-end GPU-to-peer-GPU holdout

For candidates surviving E02–E05, run paired randomized trials with exact model-like shapes/dtypes and a deterministic transformation checked on the peer GPU and CPU. Add simultaneous dual-rail, GPU compute, and NVMe contention cells. Include cancellation, rail loss, reconnect/new epoch, and buffer-pool exhaustion. Any digest/guard mismatch, stale epoch acceptance, GPU/kernel fault, or reuse-before-completion fails the candidate.

## dma-buf / kernel-extension boundary

Do not attempt dma-buf import into USB4STREAM because the pinned driver has no importer ABI. A future prototype requires a separate reviewed design covering exporter compatibility, `dma_resv`/fences, CPU sync, IOMMU/DMA mapping, invalidation, long-term pins, cgroup accounting, permissions, teardown, and upstream fallback.
