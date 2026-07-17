---
section_id: "77"
title: "HaloKV Restore, Writeback, Hit-Rate, and Endurance Benchmarks"
status: "needs-machine-validation"
last_verified: "2026-07-16"
applies_to:
  repositories: ["fewtarius/CachyLLama@6be745998f568e379ea197fcf827baec73ff9940"]
  software_versions: ["fio 3.41 documentation", "NVMe Base 2.3"]
  hardware_revisions: ["planned dual Strix Halo and per-node NVMe; exact drives open"]
related_sections: ["14", "21", "56", "57", "58", "59", "60", "61", "62", "63", "64", "65", "73", "74", "76", "78", "80"]
---

# HaloKV Restore, Writeback, Hit-Rate, and Endurance Benchmarks

## Decision-useful summary

**[VERIFIED]** The CachyLLama donor uses per-checkpoint SSD files plus an index, RAM hot/warm tiers, SSD cold state, longest-prefix matching, system-prefix reuse, and optional `fsync`. Its current format lacks checksums and atomic temp-file replacement, so it is a behavior donor and fault-test input—not a HaloKV durability specification. [S77-001][S77-002]

**[RECOMMENDATION]** Benchmark semantic outcomes: a “hit” counts only when compatible state is restored, validated, and useful. Report lookup, I/O, deserialize/map, rank coordination, validation, resumed prefill, and writeback separately.

**[RECOMMENDATION]** Treat corruption as a miss/recompute. No performance benefit can compensate for accepting invalid KV, recurrent, MTP, sampling, or RNG state.

**[OPEN]** HaloKV format, page size, durability modes, rank protocol, garbage collector, exact SSDs, endurance ratings, and production workload traces remain unresolved. This section records no HaloFPX measurements.

## Retrieval map

- [Facts and constraints](facts_and_constraints.md)
- [Design implications](design_implications.md)
- [Procedures and checks](procedures_and_checks.md)
- [Open questions](open_questions.md)
- [Sources](sources.md)

## Research split

1. **Completed source research:** donor cache behavior, POSIX sync/rename semantics, fio measurement facilities, and NVMe health/endurance counters.
2. **Required machine work:** exact drive inventory, controlled traces, restore/writeback/GC/disk-pressure matrices, parallel ranks, decode contention, fault injection, and physical-write observation.
3. **Contingent decisions:** format/page sizes, tier budgets, durability defaults, GC, quotas, write coalescing, prefetch, endurance budgets, and promotion thresholds.
