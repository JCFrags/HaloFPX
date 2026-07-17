---
title: "Fault and recovery tables"
tags: ["faults", "recovery", "decision-tables", "single-node"]
created: 2026-07-17
updated: 2026-07-17
status: design-proposal
sources: ["COORD-01", "RPC-02", "RPC-03", "RPC-06", "STORAGE-01", "STORAGE-02", "FUZZ-05"]
related: ["Partial-Rank-Failure", "Reconnect-and-Recovery", "Degraded-Mode-Behavior", "Security-Threat-Model"]
---

# Fault and recovery tables

The CSV files in `tables/` are the normative decision tables for implementation review, test generation, and incident exercises. They are deliberately machine-readable so CI can require a disposition for every new fault or recovery mechanism.

## Table inventory

| File | Purpose | Rows | Single-node verdict present? |
|---|---|---:|---:|
| `tables/fault-matrix.csv` | Crash, partition, stale-state, corruption, retry, storage, and resource failures | 26 | **Yes**, per fault |
| `tables/recovery-options.csv` | Exact reuse, delta fetch, replay, standby, one-node replica, re-shard, conversion, and forbidden partial-cache continuation | 10 | **Yes**, per mechanism |
| `tables/degraded-modes.csv` | Service behavior when authority, rank, storage, topology, or integrity is degraded | 10 | **Yes**, per mode |
| `tables/rpc-validation-matrix.csv` | Hostile-input limits and enforcement points | 24 | Not applicable |
| `tables/threat-matrix.csv` | STRIDE threats, controls, and residual risk | 10 | Not applicable |

## Interpretation rules

1. `COMMITTED` in authority is the only global checkpoint commit. Rank-local durability, a `Prepared` message, or a locally visible manifest is insufficient.
2. A recovery plan preserves verified objects and transfers only objects proved missing or corrupt. A Bloom filter, inventory hint, or peer assertion never substitutes for object-digest verification.
3. A failed rank means distributed execution stops. “Cache available” and “complete model executable” are separate predicates.
4. A `Yes` or `Conditional` single-node verdict requires all five feasibility predicates: complete weights/adapters, sufficient memory, supported one-node topology, complete/rebuildable logical state, and a new fenced generation.
5. Any unclassified failure defaults to fail-closed reuse, abort/pause of the affected session, bounded retry, and authority reconciliation.

## Recovery-option verdicts

| Recovery mechanism | Avoids blanket multi-GB transfer? | Single-node continuation? |
|---|---:|---:|
| Exact local reuse after both ranks restart | Yes | **No while the peer is absent** |
| Delta fetch for a replacement rank | Yes | **No** |
| Force-replay on a restored two-rank topology | Yes; trades bandwidth for compute | **No during the one-rank period** |
| Older checkpoint plus forced token replay | Yes | **No** |
| Full cache download to two replacements | No | **No** |
| Hot two-rank standby | Usually | **No if the standby still requires two nodes** |
| Full-model one-node hot replica | Usually | **Yes**, because it is independently complete |
| Dynamic two-to-one re-shard plus replay | Yes | **Conditional** on full model, supported topology, capacity, fencing, and deterministic rebuild |
| Verified two-to-one cache converter | Potentially | **Conditional** on an implemented, versioned, all-or-nothing conversion contract |
| Continue using only the surviving rank cache | Superficially | **No; prohibited** |

The authoritative row-level details, triggers, detection signals, safety properties, availability effects, and transfer behavior remain in the CSV files rather than this summary.
