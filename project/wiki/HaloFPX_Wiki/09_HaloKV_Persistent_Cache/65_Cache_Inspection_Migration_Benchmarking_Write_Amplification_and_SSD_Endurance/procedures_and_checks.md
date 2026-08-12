---
section_id: "65"
title: "Cache benchmark and endurance matrix"
status: "needs-machine-validation"
last_verified: "2026-07-17"
applies_to:
  repositories: []
  software_versions: []
  hardware_revisions: ["dual Strix Halo"]
related_sections: ["73", "77", "79", "80"]
---

# Procedures and checks

## Disposable-target and authorization gate

All writes, imports, migrations, compaction, deletion, corruption, endurance, and fault cases must use a pre-created disposable cache/store and scratch filesystem with exact resolved path/backing target/store UUID, isolated service/cgroup, free-space/inode/write-budget ceilings, preserved evidence and recovery access, stop conditions, cleanup, and machine-readable receipt. Refuse symlinks/path escapes and any production cache, model store, workspace, boot/root filesystem, real backup/export, deployment SSD namespace, or sole evidence copy. Read-only SMART collection may require elevated device access and must record the command/tool/database versions; raw-device writes, device faults, power tests, and physical sanitization require explicit Section 80/operator authorization and a sacrificial target.

## M65-01 benchmark matrix

| Workload | Variables | Metrics |
|---|---|---|
| lookup/hit | entry count, prefix depth, hit/miss | latency, CPU, false match |
| restore | state size/type, hot/warm/cold, rank count | TTFT, bytes, logits equality |
| store | durability mode, stream count, size | latency, fsync time, bytes |
| concurrency | readers/writers/GC/compaction | throughput, p50/p95/p99, stalls |
| long context | model/cache type/context length | restore/store scaling, memory |
| power/crash | commit point and failure type | recovered generation, quarantine |
| endurance | checkpoint cadence/retention/GC | logical/app/host/NAND writes, SMART trend |

Use a fixed non-sensitive corpus plus model/binary hashes, the bounded disposable cache, cold/warm controls, randomized repetitions, confidence intervals, and preserved raw logs. Enforce the predeclared host-write/time/temperature/error stop budget; never run an unbounded endurance loop.

## M65-02 tooling round trip

Create a disposable store with known valid/corrupt/unreachable synthetic generations. Run list/validate/export/import/migrate/compact/delete only within that resolved root. Acceptance: metadata redaction, identical valid object digests, corruption rejection, source preservation, dry-run accuracy, idempotent receipts, and successful continuation after migration.

## M65-03 SMART/endurance baseline

Record exact SSD model/serial redaction/firmware, capacity, vendor TBW warranty, temperature, percentage used, data units written/read, power-on hours, unsafe shutdowns, media errors, and error-log entries before/after controlled workload. Convert units per pinned NVMe revision; do not infer NAND WAF without controller telemetry.
