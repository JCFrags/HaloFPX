---
section_id: "75"
title: "Fabric Benchmark Design Implications"
status: "needs-machine-validation"
last_verified: "2026-07-16"
applies_to:
  repositories: []
  software_versions: []
  hardware_revisions: ["planned dual Strix Halo nodes; exact revisions open"]
related_sections: ["49", "52", "54", "55", "73", "76"]
---

# Design Implications

## Factorized matrix

**[RECOMMENDATION]** Use a staged matrix so an exhaustive sweep does not conceal basic defects:

| Stage | Links | Direction | Payload bytes | Queue depth / streams | Load |
|---|---|---|---|---|---|
| smoke | A, B | each direction | 64, 4 KiB, 1 MiB | 1 | idle |
| latency | A, B, policy-selected dual | each direction | 1, 8, 64, 256, 1 KiB, 4 KiB, 16 KiB | 1, 2, 4, 8, 16, 32 | idle |
| bandwidth | A, B | uni then bidirectional | 64 KiB, 256 KiB, 1 MiB, 4 MiB, 16 MiB | 1, 2, 4, 8, 16 | idle |
| dual-link | A+B | same-flow striping, flow pinning, alternating, independent flows | latency and bandwidth sets | 1 through saturation | idle |
| contention | all viable | both | representative control, activation, KV, expert payloads | selected knee points | decode, NVMe, CPU, mixed |
| GPU E2E | all viable | both | representative tensors plus sweep | 1 through saturation | idle and concurrent decode/NVMe |

Every cell identifies link/path, transport, message framing, buffer kind, copy path, checksum method, queue ownership, CPU/IRQ affinity, and timing domain.

## Decisive GPU test boundary

**[RECOMMENDATION]** The timed operation is:

1. rank-0 GPU produces a generation-tagged pattern;
2. producer completion/visibility is established;
3. the chosen transport moves or exposes the bytes;
4. rank-1 establishes receive visibility;
5. a rank-1 GPU kernel consumes and validates the generation and payload;
6. completion returns to the timing authority.

Report the full interval plus separately instrumented producer, sender staging, fabric, receiver staging, consumer, and synchronization components. Component sums are diagnostic and need not exactly equal wall time when work overlaps.

**[RECOMMENDATION]** Run both a copy-only control and an application-shaped consumer kernel. A checksum-only kernel proves delivery integrity; an attention/collective-shaped consumer reveals scheduling and cache interaction.

## Dual-link interpretation

**[INFERENCE]** Two links provide useful aggregate capacity only if their bottlenecks and software execution paths overlap without shifting the limit to memory bandwidth, CPU, interrupts, or serialization. A near-2x expectation is an unverified hypothesis, not a gate.

**[RECOMMENDATION]** Report dual-link efficiency as `goodput(A+B) / (goodput(A) + goodput(B))` using matched single-link runs, with uncertainty. Also report whether one link's latency or errors change while the other is saturated.

## Contention and acceptance

**[RECOMMENDATION]** Pair fabric tests with:

- fixed-rate and saturated decode on both GPUs;
- sequential and random NVMe read/write load on each rank;
- simultaneous bidirectional fabric traffic;
- controlled CPU/IRQ placement conflicts;
- transport reconnect and single-link degraded mode.

Promotion requires zero accepted corrupt generations, explicit timeout/loss accounting, repeatable raw results on both directions, and an identified single-node fallback. Performance thresholds remain **[OPEN]** until Sections 73 and 76 define workload SLOs and uncertainty rules.

## Patch decision

**[RECOMMENDATION]** Do not justify a USB4 kernel patch from iperf throughput alone. Require attribution from traces/counters showing a kernel-path bottleneck, an isolated patch delta against the same kernel/configuration, correctness/fault tests, and a clean rollback comparison. Route that decision to Section 55.
