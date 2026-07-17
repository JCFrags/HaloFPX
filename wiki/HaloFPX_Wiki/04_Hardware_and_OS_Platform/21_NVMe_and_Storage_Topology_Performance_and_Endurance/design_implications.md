---
section_id: "21"
title: "Storage design implications"
status: "needs-machine-validation"
last_verified: "2026-07-16"
applies_to:
  repositories: ["HaloFPX", "HaloKV"]
  software_versions: []
  hardware_revisions: ["two project Strix Halo nodes"]
related_sections: ["22", "28", "60", "65", "77", "80"]
---

# Storage design implications

## Placement and isolation

- **[RECOMMENDATION]** Treat model artifacts as read-mostly, cache data as replaceable but integrity-checked, and experiment evidence as durable. Give each a separately reported capacity budget.
- **[RECOMMENDATION]** Keep rank-local cache ownership. A node must miss/recompute on corrupt, incomplete, wrong-version, or wrong-rank data; it must not accept invalid state.
- **[RECOMMENDATION]** Benchmark the actual filesystem path, not a raw namespace, unless an explicitly approved destructive qualification plan exists.
- **[INFERENCE]** If model reads, cache writeback, swap, and logs share one device, tail latency under concurrent GPU inference matters more than an isolated peak sequential number.
- **[OPEN]** A dedicated cache namespace/device is justified only after topology, capacity, endurance, and mixed-load measurements.

## Durability contract

- **[RECOMMENDATION]** Publish cache content through write-to-temporary, data sync, atomic rename, and containing-directory sync, with checksums and version/rank identity.
- **[RECOMMENDATION]** Recovery must discard an incomplete generation. Keep the previously committed manifest until the new one is durable.
- **[RECOMMENDATION]** Define at least `ephemeral`, `batched`, and `durable` modes. The caller must know the acknowledged durability boundary.
- **[OPEN]** Drive volatile-write-cache and power-loss-protection properties need exact-controller evidence; do not assume enterprise semantics.

## Performance and endurance gates

| Gate | Required evidence | Decision affected |
|---|---|---|
| Cold model read | aligned sequential read throughput and p50/p95/p99 latency | startup and model placement |
| Metadata lookup | file/path/index workload at realistic entry count | cache index layout |
| Concurrent inference | same storage tests during steady GPU load | service isolation and queue policy |
| Sustained writeback | 30-60 minute write/GC trace with temperature and SMART deltas | writeback rate and cooling |
| Recovery | controlled interruption with checksum/manifest assertions | durability default |
| Endurance | workload-attributed host-write delta for a representative day | cache size, TTL, GC, reserve |

**[RECOMMENDATION]** Compare nodes using matched job files, fill state, free-space fraction, temperature/ambient, firmware, kernel, filesystem, mount options, and scheduler. A result without these controls remains local diagnostic evidence, not a design fact.
