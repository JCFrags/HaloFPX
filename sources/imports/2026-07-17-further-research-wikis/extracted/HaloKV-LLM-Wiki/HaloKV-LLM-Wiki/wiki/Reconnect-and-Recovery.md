---
title: "Reconnect and recovery"
tags: ["reconnect", "recovery", "delta-transfer"]
created: 2026-07-17
updated: 2026-07-17
status: design-proposal
sources: ["KV-02", "KV-03", "KV-06", "RPC-03"]
related: ["Partial-Rank-Failure", "Degraded-Mode-Behavior", "Integrity-and-Corruption"]
---

# Reconnect and recovery

## Reconnect handshake

A reconnecting process sends a bounded `Hello` containing:

- authenticated node/workload identity and claimed logical rank;
- protocol and cache-format ranges;
- sender instance ID and boot nonce;
- session/generation and highest persisted authoritative epoch;
- exact-reuse and transport-compatibility fingerprints;
- last observed commit certificate ID/revision;
- local inventory Merkle root, page/object counts, and optional bounded Bloom filter;
- local durability watermark and corruption/quarantine summary.

The coordinator authenticates the process, derives its permitted rank, queries authority, and returns a signed/authorized plan. It never trusts a peer’s last epoch or certificate as proof.

## Reconnect outcomes

| Outcome | Preconditions | Action |
|---|---|---|
| `RESUME_EXACT` | current generation/epoch, exact topology, same committed checkpoint, all required local pages verified | attach at barrier and continue with both ranks |
| `FETCH_DELTA` | compatible topology and authoritative checkpoint; finite missing/corrupt set | fetch only missing objects, verify, then attach |
| `REBUILD_RANK` | compact semantic recovery state exists; topology can execute prefill/replay | force-replay prompt and emitted tokens for missing rank/state |
| `RECONFIGURE` | full model fits alternate topology and engine supports it | new generation/epoch; rebuild or verified convert |
| `RESET_SESSION` | no safe checkpoint/replay path | discard cache and restart from source request policy |
| `REJECT_STALE` | obsolete generation/epoch or superseded instance | fence and terminate old process |

## Delta inventory protocol

1. Compare global/rank manifest IDs first. Equal immutable IDs require no page listing.
2. For unequal manifests, compare subtree/Merkle roots by layer or page range.
3. Use a Bloom filter only to suppress likely-present page IDs; false positives are discovered during final verification/materialization.
4. Produce a bounded recovery plan token naming missing page IDs or paginated ranges.
5. Fetch from the closest authorized source: local store, peer/off-host replica, then independent blob store.
6. Verify each page’s digest and structural header before marking it present.
7. Recompute only the remaining unavailable rank ranges when supported.

This avoids copying the survivor’s rank cache simply to recreate symmetry. The replacement needs its own rank’s objects, plus any shared metadata.

## Compact recovery capsule

Persist a small capsule alongside the certificate:

```text
model/topology/input fingerprints
canonical prompt/input references or authorized retrieval token
committed logical position
ordered emitted token IDs after prompt
sampler and beam state required for exact continuation
sequence IDs, masks, position/sliding-window bases
last externally acknowledged output offset
```

The capsule is sufficient to orchestrate deterministic force-replay; it is not a KV cache and is not enough for one incomplete rank to generate alone.

## Replay semantics

After failure, already emitted tokens are facts. Rebuild by feeding the original prompt and then force-feeding those token IDs through the restored topology. Do not sample them again. The implementation must define whether numerical/cache reconstruction is exact enough across hardware and kernels; otherwise start a new generation and treat the rebuilt state as semantically continuous but byte-distinct.

For tensor parallelism, both restored shards generally participate in replay. For pipeline parallelism, all stages are needed unless intermediate activations and complete stage state were independently preserved. Replaying on the survivor alone is not single-node continuation unless it also has the missing weights and a supported full-model topology.

## Half-open and duplicate connections

Application heartbeats detect liveness but do not grant ownership. A new connection for the same logical rank and epoch is accepted only after comparing boot nonce and authority lease. The older instance is fenced; simultaneous active instances are a security event. Delayed frames from the old connection carry the stale instance/epoch and are rejected.

## Recovery transfer accounting

Expose `logical_checkpoint_bytes`, `bytes_already_local`, `bytes_fetched`, `bytes_recomputed`, and `bytes_avoided`. “Bytes avoided” is meaningful only after integrity verification; Bloom-filter guesses do not count.
