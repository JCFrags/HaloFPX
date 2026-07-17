---
section_id: "52"
title: "Dual-Link Multipath - Procedures and Checks"
status: "needs-machine-validation"
last_verified: "2026-07-17"
applies_to:
  repositories: ["linux@fce2dfa"]
  software_versions: ["Linux MPTCP"]
  hardware_revisions: ["two target USB4 links"]
related_sections: ["20", "49", "50", "53", "55"]
---

# Procedures and checks

## Internet/source-code research completed

Reviewed MPTCP protocol sequencing/reinjection, Linux path-manager/scheduler controls, and QUIC sequencing/reordering principles. No external benchmark was treated as target evidence.

## FT-52-E1 - physical/logical independence

Map each cable, controller, XDomain, interface, IRQ, NUMA/CPU locality and link speed. Run link A only, B only, and simultaneous A+B in both directions. Independence requires stable enumeration and a documented simultaneous-load result; two connectors alone are insufficient.

## FT-52-E2 - policy sweep

For payloads `64 B, 256 B, 1 KiB, 4 KiB, 16 KiB, 64 KiB, 256 KiB, 1 MiB, 4 MiB, 16 MiB` and actual activation sizes, compare:

- best A / best B;
- round-robin operations;
- direction separation;
- equal and measured-proportional striping with multiple chunk sizes;
- MPTCP scheduler/path-manager baseline;
- hedge off/on only for eligible small messages.

Capture p50/p95/p99/p99.9 completion, aggregate and per-link goodput, reorder bytes/time, duplicate bytes, CPU/IRQ cost, queue delay, deadline misses, and fairness with control traffic.

## FT-52-E3 - degradation and failover

Safety boundary: run delay, reorder, corruption, and receiver-stall cases first in an unprivileged disposable harness. A worker restart must target a dedicated test process with no production/model state. Cable removal and administrative interface changes require an approved Section 80 plan naming the exact resolved rail/interface, privileges, out-of-band recovery path, resource ceilings, stop conditions, teardown, and post-test baseline smoke. Never expose production traffic, a boot/storage path, model/cache store, workspace, or sole evidence copy.

During steady test traffic, remove one authorized exact cable, administratively down one authorized interface, stall one receiver, inject delay/jitter, restart one test worker, and restore the link. Measure detection, last confirmed operation, barrier time, old-epoch failures, new-epoch establishment, retry disposition, control-lane availability, and whether any stale generation is applied. Passing requires that partial old-epoch records are never retransmitted on the survivor; only explicitly idempotent whole operations may retry after the fresh epoch is active.

## FT-52-E4 - correctness oracle

Generate deterministic messages with per-chunk and whole-message digests. Intentionally reorder, duplicate, omit, overlap, corrupt, and deliver old-generation chunks. Pass only if output is exact or the message fails explicitly; partial/corrupt data must never be accepted.

## Decision rule

**[RECOMMENDATION]** Select the simplest policy whose confidence intervals meet accepted SLOs for the real traffic mix. Record thresholds with kernel, firmware, cabling, CPU affinity, and thermal context; invalidate them when those change materially.
