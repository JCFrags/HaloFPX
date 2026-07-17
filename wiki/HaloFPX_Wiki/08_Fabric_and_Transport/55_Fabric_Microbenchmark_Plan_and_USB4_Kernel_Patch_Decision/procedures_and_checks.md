---
section_id: "55"
title: "Fabric microbenchmark program and decision procedure"
status: "needs-machine-validation"
last_verified: "2026-07-17"
applies_to:
  repositories: ["torvalds/linux@8cdeaa50eae8dad34885515f62559ee83e7e8dda"]
  software_versions: ["Linux 7.1.3 historical baseline", "Linux 7.2-rc2 candidate"]
  hardware_revisions: ["two Nimo MME3L Strix Halo nodes; exact revisions open"]
related_sections: ["20", "50", "52", "53", "54", "73", "75", "84"]
---

# Procedures and checks

No step changes a node until its reversible plan, exact resolved targets, package/source hashes, declared privileges, resource/free-space/thermal ceilings, stop conditions, cleanup, console/out-of-band recovery, and exclusions are approved. Candidate kernels install alongside a known-good boot entry; no in-place overwrite is authorized. Faults and physical rail changes require Section 80 authorization. Use only disposable generated payloads and dedicated test services; never target a production model/cache store, boot/storage path, workspace, or sole evidence copy. Do not build kernels on inference nodes.

## S55-E01 — Admission and kernel crossover

1. Capture exact BOM/port/cable map, firmware, kernel/config/module hashes, IOMMU/security, GTT, USB4 domains/HopIDs, routes, offloads, IRQs, clocks, power, thermals, services, logs, and management recovery.
2. Install a reviewed candidate kernel alongside the known-good image; stagger reboots and smoke storage, amdgpu/ROCm, both USB4NET rails, management, and rollback.
3. Run a preregistered old↔candidate kernel crossover using identical USB4NET fixture. Use at least 12 complete reboot pairs, balanced order. Require every throughput lower confidence bound ≥0.95 and every p99 upper bound ≤1.10; zero new errors, resets, fallback, or rail drift.

## S55-E02 — Codec/backend equivalence

Run the Section 53 record/credit/integrity implementation over same-kernel bound TCP and USB4STREAM. Verify identical corpus digests, short-I/O state machines, cancellation, epochs, and metrics. Complete authenticated drain, HopIDs→0, peer-property disappearance, device removal, module cleanup, and post-cleanup USB4NET smoke.

## S55-E03 — Full matrix

Execute the payload/queue/direction/rail/batch axes in [facts_and_constraints.md](facts_and_constraints.md). Randomize paired TCP↔STREAM order. Each latency member retains ≥100,000 observations or a lossless verifiable histogram. Freeze an even `N >= 20` complete-pair count for confirmatory endpoints; do not stop or extend after viewing results.

## S55-E04 — Contention and GPU-to-peer-GPU

Repeat selected real-trace cells under isolated GPU, NVMe, CPU-memory, and safe combined contention. Capture per-component overlap, stalls, PSI, clocks/power/temperature and digest correctness. Do not mix contention cells into idle estimates.

## S55-E05 — Correctness and failure soak

Run ≥1,000,000 records per candidate topology/direction with zero corruption, loss, duplication, unaccounted reorder, stale-epoch acceptance, deadlock, or unbounded memory. Run ≥10 reconnect cycles for each advanced rail/failure mode. Any kernel/GPU fault, silent management-LAN fallback, residual ConfigFS/device state, or failed rollback rejects the candidate.

## S55-E06 — Paired holdout decision

For each whole randomized pair `i`, calculate STREAM/TCP positive-metric ratios. Bootstrap the `N` whole pairs 100,000 times with a recorded seed and median-ratio estimand; do not resample individual messages. Apply one-sided family-wise alpha 0.05/3 across the three alternative benefit families.

Accepted D-035 advancement alternatives (all named endpoints and both directions are conjunctive):

| Alternative | Required adjusted one-sided bound |
|---|---|
| Latency | STREAM/TCP p50 and p99 upper bounds ≤0.85 |
| Real end-to-end | STREAM/TCP real-trace throughput lower bound ≥1.15 |
| CPU | combined two-host cycles/delivered-byte upper bound ≤0.80, throughput lower bound ≥0.98, p99 upper bound ≤1.05 |

## Registered-buffer extension gate

**[RECOMMENDATION]** Open a separate patch proposal only if all are true:

1. E01/E02/E05 upstream correctness, security, cleanup, and rollback pass.
2. Upstream USB4STREAM misses all E06 advancement alternatives on the real trace.
3. Approved tracing in both directions shows iterator copy/cache-sync work accounts for at least 20% of combined two-host CPU cycles in the failing primary cells, with no larger non-copy bottleneck.
4. A minimal registered-buffer prototype preserves the exact wire protocol and, on a fresh paired holdout, achieves either ≥15% real end-to-end improvement or ≥20% CPU-cycle reduction while retaining throughput ≥98% and p99 ≤105% of upstream USB4STREAM.
5. One-million-record correctness, pinned-memory accounting, invalidation, IOMMU/DMA isolation, permission, teardown, rail-failure, and old-interface fallback all pass.

Failure of any item closes the patch branch and retains upstream interfaces. Passing authorizes an upstreamable proposal, not production deployment.
