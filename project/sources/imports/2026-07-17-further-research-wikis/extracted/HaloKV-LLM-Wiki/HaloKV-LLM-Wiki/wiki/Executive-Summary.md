---
title: "Executive summary"
tags: ["architecture", "invariants", "recovery"]
created: 2026-07-17
updated: 2026-07-17
status: design-proposal
sources: ["KV-01", "KV-03", "KV-06", "COORD-01", "RPC-03"]
related: ["System-Model", "Checkpoint-Commit-Protocol", "Partial-Rank-Failure", "Degraded-Mode-Behavior"]
---

# Executive summary

## Recommended design

HaloKV should persist rank-local KV pages as immutable, content-addressed objects and publish only a small, coordinated global manifest as the checkpoint commit point. A checkpoint becomes readable when an external authority atomically publishes a commit certificate that names a validated global manifest containing both expected ranks at the same logical token position.

The two execution nodes should not also be the sole fault-tolerant authority. A two-member quorum needs both members and therefore tolerates zero failures. Use an independent epoch/commit authority with a documented compare-and-swap contract, normally a three-voter consensus service. When that authority cannot be reached, existing inference may continue only under an explicitly bounded lease policy that does not create new persistent commits; conservative deployments pause before producing state whose ownership could become ambiguous.

## Required invariants

| ID | Invariant |
|---|---|
| H-1 | A committed checkpoint names exactly the expected rank set and one logical position. |
| H-2 | A mutating message is accepted only under the receiver’s current authoritative generation and epoch. |
| H-3 | Cache bytes are reused only under an exact-reuse topology fingerprint or an explicitly implemented conversion path. |
| H-4 | A page is verified structurally and cryptographically before GPU materialization or attention use. |
| H-5 | Each checkpoint operation has one terminal outcome: committed or aborted. Ambiguous client observation is resolved by querying authority, not by guessing. |
| H-6 | In-flight bytes, pages, operations, decompression work, and persisted orphan state are bounded. |
| H-7 | Recovery never silently changes already emitted tokens. Tokens emitted after the last checkpoint are force-replayed, not resampled. |
| H-8 | The advertised durability class matches where every required rank object actually resides. |

## State and identity

Every mutation carries `tenant_namespace`, `session_id`, `session_generation`, `epoch`, `checkpoint_seq`, `op_id`, sender instance, logical rank, topology fingerprint, and request digest. Receivers persist the highest authoritative epoch before accepting work. Lower epochs are stale; higher epochs require proof from the authority rather than peer assertion.

Use two identities:

- A privacy-preserving logical lookup key, tenant-scoped with HMAC and covering all inputs that affect KV values.
- A cryptographic immutable page ID over a canonical page header and bytes.

Physical host identity belongs in inventory metadata, not in the immutable page ID, so a logical rank can relocate without invalidating otherwise compatible content.

## Recovery without blanket transfer

Reconnect starts with a small `HELLO` and inventory summary. The coordinator selects among exact resume, delta fetch, rank rebuild, session reset, or topology reconfiguration. A Merkle root and bounded Bloom filter can reduce inventory exchange, but they are hints; every chosen page is verified by digest.

The normal recovery unit is the **missing rank**, not the whole checkpoint. Preserve verified local pages on the surviving node, fetch only absent/corrupt pages for the replacement, or reconstruct the missing rank by force-replaying the prompt and already emitted tokens. Recompute avoids a multi-gigabyte transfer but consumes compute and still requires a complete executable topology.

## Single-node continuation verdict

> [!danger] Usually no
> Neither rank-local persistence nor shared object storage makes one model-parallel rank a complete model. A surviving node may continue alone only when it has all weights/layers/heads required by the model, sufficient memory and kernels for a supported one-node topology, and the full logical KV state in that topology—or can deterministically rebuild that state locally.

Dynamic repartition may make continuation possible, but rank-local cache pages will usually be layout-incompatible; the safe path is to rebuild from a compact recovery capsule containing input identities, token history, logical position, sampler state, and committed output history.

## Verification strategy

Use TLA+/TLC as the primary protocol model for atomic commit, epoch fencing, cancellation races, reconnect, stale messages, and corruption rejection. Use Apalache for bounded symbolic checks where useful, P for implementation-near communicating-state-machine and trace-conformance work, and Alloy only for finite relational checks such as topology/fingerprint compatibility. Pair the model with coverage-guided schema fuzzing, stateful model-based sequence fuzzing, crash-point enumeration, network fault injection, deterministic concurrency tests, and silent-corruption campaigns.
