---
title: Integration Order
description: Phase-by-phase delivery plan with entry gates, exit gates, and rollback points.
status: Proposed
evidence-date: 2026-07-17
canonical-repository: charlie12345/ROCmFPX
---

# Integration Order

> [!NOTE]
> Evidence is pinned to **2026-07-17**. Repository heads and source links are locked in [[Source-Register]] and `evidence/commit-lock.json`.


## Phase 0 — Confirm identity, provenance, and baseline

**Lanes:** L00, L14

**Entry:** none.

**Work:**

- maintainer confirms `fewtarius/CachyLLama` as intended donor;
- lock exact canonical/upstream/donor heads;
- identify exact donor introduction commits for selected files;
- classify upstream-equivalent patches;
- establish clean build/test baseline on canonical head;
- create signed `integration-base/*` tag and rollback branch.

**Exit:** all selected features are P2 or explicitly designated clean-room; baseline matrix green.

**Rollback:** delete lane branches; no code or format exists.

## Phase 1 — Contracts and provider seam

**Lanes:** L01 → L02

**Work:** codify current per-run cache behavior, add provider/state capability contracts, wrap current ROCmFPX cache in an adapter, expose feature-state metrics.

**Exit:** feature-off and current-adapter behavior match baseline; every cache test remains green; no persistent writer exists.

**Rollback point:** `cache-provider-seam-v0`. Revert L02 only; L01 tests can remain.

## Phase 2 — Format reader before writer

**Lanes:** L03 → L04

**Work:** bounded v1 manifest/component reader, incompatibility/quarantine behavior, fuzz corpus, then disabled atomic writer.

**Exit:** parser survives malformed/truncated/oversized inputs; unknown majors/features reject; crash/disk-full tests prove no partial committed entry.

**Rollback points:** `cache-format-v1-reader`, then `cache-format-v1-writer-off`.

## Phase 3 — Persistent provider canary

**Lanes:** L05 → L06

**Work:** persistent read-only canary, then read-write canary; retention; prefetch; independent match policy; optional offline importer only after canonical v1 is stable.

**Enablement sequence:**

1. compiled, runtime off;
2. read-only on test fixtures;
3. read-only on canary production nodes;
4. write-only shadow validation to a disposable store;
5. read-write for a bounded canary population;
6. opt-in stable.

**Exit:** restart reuse, corruption fallback, quota, and rollback drills pass.

**Rollback point:** `persistent-cache-v1-canary`; switch provider to `ephemeral` without deleting the persistent store.

## Phase 4 — System-prefix and hybrid state

**Lanes:** L07 and L08

These lanes integrate only after the base store is stable. L08 may land its internal capability contract earlier, but persistent hybrid restore remains off until target/draft/spec/recurrent round-trip tests pass.

**Exit:** exact-boundary restore is deterministic; cross-conversation recurrent safety tests pass; system prefix boundaries are explicit or template-verified; heuristic mode remains off.

**Rollback:** disable L07/L08 flags independently; retain ordinary persistent transformer entries if still valid.

## Phase 5 — Identity and scheduling

**Lanes:** L09 → L10 → L11

- L09 carries scope identity only.
- L10 adds concurrency cap/429 behavior.
- L11 adds optional slot-affinity scoring.

**Exit:** no cross-scope cache access, race-safe counters, fair anonymous handling, and feature-off scheduler equivalence.

**Rollback:** disable affinity first, then cap, then explicit scope caching. Do not reinterpret existing scoped entries as anonymous.

## Phase 6 — Telemetry and clean-room build presets

**Lanes:** L12, L13

Ship independently. Expert telemetry remains compile/runtime off until overhead and ABI gates pass. ISA detection is an ROCmFPX-owned implementation based on an approved black-box requirements document.

## Phase 7 — Compatibility and default policy

**Lane:** L15

Only after at least two release cycles of opt-in use:

- decide whether any donor CLI aliases are justified;
- decide whether persistent support remains compile-time optional;
- consider public C API promotion;
- consider a default-on policy through a separate release ADR.

## Deferred research

Page-level SSD paging is not on the critical path. It requires its own maturity/provenance review, performance model, and failure-isolation design before it can become a lane.


---

**Wiki navigation:** [[Home]] · [[Executive-Decision]] · [[Capability-Decision-Matrix]] · [[Patch-Lanes-and-Dependency-Graph]] · [[Acceptance-Criteria]]
