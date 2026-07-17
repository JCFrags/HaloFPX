---
title: "Decision log"
tags: ["adr", "decisions"]
created: 2026-07-17
updated: 2026-07-17
status: design-proposal
sources: ["COORD-01", "KV-03", "STORAGE-01"]
related: ["Open-Questions", "Executive-Summary"]
---

# Decision log

## D-001 — External epoch and commit authority

**Decision:** Do not use the two execution nodes as the sole available quorum. Use an independent strongly consistent authority for monotonic epochs, terminal operation records, and conditional commit-certificate publication.

**Reason:** A two-member majority requires both members and tolerates zero failures. External fencing also prevents an isolated stale process from publishing state after replacement.

**Rejected:** rank 0 is always leader with no external fence; wall-clock leases stored only on execution nodes; last-writer-wins object names.

## D-002 — Immutable rank pages and manifests

**Decision:** Pages, rank manifests, and global manifests are immutable and content-addressed. Only small indexes/authority records are mutable.

**Reason:** Idempotent retries, deduplication, delta recovery, orphan safety, and end-to-end integrity become simpler.

## D-003 — Global certificate is the commit point

**Decision:** Both ranks prepare at one logical position; the authority CAS of the global certificate linearizes commit.

**Rejected:** “rank 0 committed” marker; newest timestamp; directory scan; assuming two local prepare files imply global commit.

## D-004 — Exact topology fingerprinting

**Decision:** Direct reuse requires an exact-reuse fingerprint. Transport compatibility and conversion are separate contracts.

**Reason:** Cache shape and values depend on model, shard mapping, layout, dtype, attention semantics, adapters, and inputs beyond tokens.

## D-005 — Fail closed on incomplete or corrupt state

**Decision:** Missing rank, stale epoch, digest failure, malformed shape, or topology mismatch produces rejection/rebuild, not partial reuse.

## D-006 — Application byte/page credits

**Decision:** Retain transport flow control and add bounded application credits, quotas, watermarks, and reserved control capacity.

## D-007 — Recovery unit is missing content, usually one rank

**Decision:** Compare immutable manifests/inventories and fetch only absent/corrupt pages. Preserve the surviving rank’s verified cache.

## D-008 — Single-node continuation is conditional, not a cache feature

**Decision:** Report single-node continuation as impossible unless the node has a complete executable model and complete/rebuildable logical cache under a supported one-node topology.

**Reason:** Storage availability does not supply missing layers, heads, experts, collectives, or compatible tensor layout.
