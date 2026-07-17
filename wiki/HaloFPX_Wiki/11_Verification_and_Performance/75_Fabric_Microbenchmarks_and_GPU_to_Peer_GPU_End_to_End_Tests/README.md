---
section_id: "75"
title: "Fabric Microbenchmarks and GPU-to-Peer-GPU End-to-End Tests"
status: "needs-machine-validation"
last_verified: "2026-07-17"
applies_to:
  repositories: []
  software_versions: ["iperf3 3.21", "HIP 6.4.x", "Linux 6.15 documentation"]
  hardware_revisions: ["planned dual Strix Halo nodes and two USB4 links; exact revisions open"]
related_sections: ["20", "24", "49", "50", "51", "52", "53", "54", "55", "73", "76"]
---

# Fabric Microbenchmarks and GPU-to-Peer-GPU End-to-End Tests

## Decision-useful summary

**[VERIFIED]** Standards distinguish one-way delay, round-trip delay, delay variation, loss, link capacity, and link usage. They also require the packet type, loss threshold, path, clock uncertainty, and calibration to accompany reported delay results. These distinctions apply to HaloFPX even though its path is local and specialized. [S75-001][S75-002][S75-003]

**[RECOMMENDATION]** Qualify link A, link B, both links independently, and both links under simultaneous load. For every topology, sweep payload size and queue depth and retain raw per-operation observations rather than only averages.

**[RECOMMENDATION]** Treat host-network throughput as a lower-layer diagnostic, not the acceptance test. The decisive test starts with GPU-produced bytes on rank 0 and ends only after a rank-1 GPU kernel has consumed and validated them, with synchronization included in the timed interval.

**[MEASURED]** A bounded reachability diagnostic on 2026-07-17 sent five ICMP requests in each direction over each private rail. All 20 requests succeeded, with per-direction/per-rail mean RTTs from 0.087 to 0.100 ms [S75-L01]. This is not a promoted fabric benchmark: sample size, ICMP path, warm system state, and lack of controlled counter deltas do not satisfy this section's experiment contract.

**[OPEN]** USB4 path independence, achieved goodput, p95/p99 tails, GPU-visible buffer behavior, simultaneous scaling, and the benefit of two links remain machine questions.

## Retrieval map

- [Facts and constraints](facts_and_constraints.md) defines metrics and source-backed tooling behavior.
- [Design implications](design_implications.md) defines the matrix, timing boundary, and promotion gates.
- [Procedures and checks](procedures_and_checks.md) is the non-destructive execution plan.
- [Open questions](open_questions.md) records hardware and implementation blockers.
- [Sources](sources.md) is the primary-source ledger.

## Research split

1. **Internet/source research completed now:** standardized delay vocabulary; TCP test controls; iperf3 modes; Linux interface/timestamping observability; HIP synchronization semantics.
2. **Required on-machine work:** inventory the exact links and driver paths, calibrate clocks/timers, execute all host and GPU end-to-end cells, add contention, and retain raw evidence.
3. **Contingent decisions:** transport, striping policy, queue depth, payload chunking, buffer placement, synchronization protocol, and whether a USB4 kernel patch is justified.

No benchmark number in a repository, standard, or vendor document is promoted as a HaloFPX result.
