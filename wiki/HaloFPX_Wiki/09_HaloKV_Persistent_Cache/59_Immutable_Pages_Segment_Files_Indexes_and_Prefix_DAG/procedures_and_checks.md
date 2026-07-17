---
section_id: "59"
title: "HaloKV format prototype procedures and checks"
status: "needs-machine-validation"
last_verified: "2026-07-17"
applies_to:
  repositories: ["HaloKV design"]
  software_versions: ["format proposal v0"]
  hardware_revisions: ["dual Strix Halo NVMe hosts; exact filesystems pending"]
related_sections: ["57", "58", "62", "63", "65"]
---

# HaloKV format prototype procedures and checks

## Safety boundary

Golden-vector and read-only parser work needs no root access. Compaction, truncation, corruption, ENOSPC, crash, reboot, power-cut, and device-fault cases require the disposable-target gate in Section 63: exact resolved scratch path/store UUID, isolated service/cgroup, resource ceilings, preserved recovery access, stop conditions, cleanup, and evidence receipt. Never target the live cache, model store, workspace, boot/root filesystem, or sole evidence copy. Physical/kernel/device faults require Section 80 authorization.

## Deterministic format prototype

The future format tool must provide `--help`, `inspect`, `verify`, `rebuild-index`, `salvage-plan`, `compact-plan` and explicit write commands; inspection is the default and never mutates data. It must enforce checked arithmetic before allocation/read and emit JSON plus human diagnostics.

Build golden vectors for:

- zero/one/many pages; each component/schema/rank;
- page padding/alignment and segment sealing;
- parent/child branches and shared prefix lookup;
- canonical manifests/fingerprints from section 57;
- unknown optional and required fields;
- every byte-order and integer boundary.

Two independent readers should reproduce object IDs and reject malformed vectors identically.

## Machine matrix

1. Inventory filesystem, mount options, block sizes, NVMe atomic/write-cache characteristics and kernel.
2. Test candidate page/segment sizes with real serialized component distributions, not synthetic 4 KiB-only traffic.
3. Compare buffered `pread`, mmap, readahead and aligned direct/async I/O for cold/warm restores; capture faults, page cache, NVMe queues, CPU and tail latency.
4. Branch many conversations from a shared prefix and measure metadata/index size, lookup complexity and physical deduplication.
5. Compact live/dead mixes while readers run. Verify readers see either old or new committed references, never missing/mixed pages.
6. Rebuild indexes solely from committed manifests/segments and compare canonical logical inventory.

## Corruption/fault corpus

Flip/truncate/duplicate/reorder headers, page payloads, footer, WAL records and DAG links; inject cycles, broken token intervals, wrong ranks, stale generations, digest collisions in mocked hash, huge lengths and path traversal. Every invalid object must become a quarantined miss or explicit fatal metadata error—never inference state.

Crash/power-cut at each publication step on an approved disposable filesystem matching the candidate deployment, using the Section 63 harness. For every mode, verify recovery rejects/quarantines incomplete generations and recomputes. For turn-durable/strict modes only, additionally verify that acknowledged generations have no dangling references under the mode's declared and tested failure model.

## Compaction acceptance

- Plan records input/output digests and expected live set.
- New segments are fully verified before metadata swap.
- Reference transaction commits before old segment deletion.
- Open readers remain safe; deletion waits for generation/lease policy.
- Interrupted compaction is idempotently resumed or discarded.
- Logical checkpoint and prefix IDs do not change.

No page size, I/O method or compaction threshold is approved without raw two-host results.
