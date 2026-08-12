---
title: "Partial-rank failure"
tags: ["failure", "model-parallelism", "single-node"]
created: 2026-07-17
updated: 2026-07-17
status: design-proposal
sources: ["COORD-01", "KV-01", "KV-06"]
related: ["Reconnect-and-Recovery", "Degraded-Mode-Behavior", "Checkpoint-Commit-Protocol"]
---

# Partial-rank failure

## Immediate response

When a collective, heartbeat, transport, GPU, process, or host failure indicates one rank is unavailable:

1. stop scheduling new decode steps for the session;
2. abort or tear down the distributed communicator;
3. cancel in-flight checkpoint/data transfers at safe points;
4. fence the old execution epoch through the authority;
5. retain the last authoritative commit certificate and externally emitted token log;
6. choose replacement, rebuild, reconfiguration, or reset.

Do not checkpoint at an arbitrary CUDA/NCCL failure point. A rank’s local tensors may reflect a partially completed logical step even when their memory is readable.

## Visibility boundary

The last committed cache position and the last token delivered to the client may differ. Recovery must preserve client-visible history:

- Tokens at or before the checkpoint are represented by committed state.
- Tokens emitted after the checkpoint are force-replayed during reconstruction.
- Tokens computed but not externally acknowledged may be discarded according to the streaming acknowledgment contract.
- Sampling state, beam state, speculative-decoding acceptance, and sequence positions must be restored consistently.

## Recovery paths

| Mechanism | Avoids full two-rank cache transfer? | Service resumes when | Single-node continuation? |
|---|---:|---|---:|
| Replace failed rank; fetch only its pages | yes | replacement has missing rank weights/cache and rejoins | **No** while only survivor runs |
| Replace failed rank; force-replay its state | yes, trades transfer for compute | complete two-rank topology has replayed to visible boundary | **No** while only survivor runs |
| Fetch both ranks from shared store | not necessarily | both ranks restored | **No**; shared storage is not compute |
| Use a hot standby with full model and full logical KV | may avoid transfer | standby is fenced in and verified | **Yes**, only if standby is one-node capable or supplies all ranks |
| Dynamically re-shard to one node and rebuild from tokens | yes for KV, costly compute | full model fits and engine supports one-node topology | **Conditionally yes** |
| Reuse survivor’s rank-local pages unchanged after two-to-one re-shard | usually no | only through a verified layout converter | **Not by default** |
| Continue decoding with the survivor’s partial heads/layers | no safe mechanism | never | **No** |

## Parallelism-specific implications

**Tensor parallelism:** each rank normally owns only a subset of weights/heads and participates in every layer’s collectives. One rank cannot produce the correct logits alone. Single-node continuation requires loading the missing shards and rebuilding/transforming complete KV.

**Pipeline parallelism:** the survivor normally lacks some layers. It cannot complete a forward pass. Persisted boundary activations may shorten replay but do not replace missing layers for subsequent tokens.

**Expert parallelism:** availability depends on routing and whether all experts required by future tokens are present. Treat loss of any declared topology member as non-executable unless the model has an explicit degraded expert-routing contract; that would be a different model semantics and generation.

## Replacement safety

A replacement receives a new process identity and authoritative epoch. It may assume the failed logical rank number only after authorization. Before attaching to collectives it verifies the exact topology, global manifest, rank manifest, page integrity, logical position, and peer’s matching attach barrier.

The old rank remains fenced even if it reconnects with intact cache. Its pages may be imported as immutable content after validation, but it cannot mutate the current generation under the stale epoch.

## Availability policy

A deployment that requires uninterrupted service after one execution-node loss must provision actual redundancy: a complete one-node-capable replica, a replacement rank with sufficiently fast state restoration, or more execution replicas. Persistent rank-local cache alone is a recovery optimization, not high availability.
