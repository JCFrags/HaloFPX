---
section_id: "62"
title: "Async I/O experiments"
status: "needs-machine-validation"
last_verified: "2026-07-17"
applies_to:
  repositories: []
  software_versions: []
  hardware_revisions: ["dual Strix Halo"]
related_sections: ["55", "73", "77", "79"]
---

# Procedures and checks

## Isolation and privilege gate

Benchmark only a pre-created disposable cache file/store on a separately resolved scratch filesystem, with an isolated service/cgroup, explicit byte/inode/memory/I/O ceilings, preserved out-of-band recovery, stop conditions, cleanup, and evidence receipt. Refuse production cache/model/workspace/boot paths and sole evidence copies. Normal service and fixture tests require no root access. Profiler setup, cgroup configuration, kernel fault injection, device faults, or EIO simulation must declare the minimum privilege, use Section 80 authorization, and never alter the deployment device or filesystem.

## M62-01 I/O matrix

Compare synchronous buffered baseline, io_uring buffered, and aligned direct I/O at checkpoint sizes/fragmentation representative of target models. Sweep queue depth and registered/unregistered buffers. Record submit/completion latency, CPU, throughput, page faults/cache residency, memory, cancellation latency, and server TTFT/p95 decode impact.

## M62-02 prefetch precision

Trigger from incremental tokenization at multiple prefix lengths. Record candidates, useful bytes, wasted bytes, hit latency, page-cache displacement, and false-positive rate under concurrent sessions.

## M62-03 GPU staging

Compare pageable staging, pinned/registered host buffers, and any supported unified/GPU-visible mapping. Verify exact restored logits and capture CPU/GPU synchronization and copy timelines. No root required for service runs; profiler/kernel feature setup may require administrator access and must be documented.

## Failure checks

In the isolated disposable target only, cancel reads during slot reuse, truncate fixture files during restore, bound and saturate scratch writeback, inject ENOSPC/EIO through an approved loopback/fault harness, and pressure the test cgroup rather than the host. Acceptance: bounded resources, no stale completion publication, active inference remains responsive, and failures become misses/recompute.
