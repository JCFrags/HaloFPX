---
title: Ownership matrix
status: normative placement summary
---

# Ownership matrix

Ownership is part of correctness. “The model is distributed” is incomplete until every stateful and decision-making component has one defined owner or an exact replication protocol.

## Role matrix

| Mode | Rank 0 / node A | Rank 1 / node B | Tokenizer owner | Final sampler / RNG owner | Model ownership | Expert ownership | KV ownership | Session owner |
|---|---|---|---|---|---|---|---|---|
| **TP=2** | Coordinator + tensor rank 0 | Tensor rank 1 | Rank 0 | Distributed vocabulary protocol; rank 0 final authority | Every layer tensor-sharded 0/2 and 1/2; embedding/head compatibly sharded | Separate hybrid required for MoE | KV-head shard per rank when divisible; otherwise explicit variant | Rank 0 authoritative; rank 1 mirrors token IDs |
| **Contiguous split** | Embeddings + layers \([0,k)\) | Layers \([k,L)\) + norm/head | Rank 0 | Rank 1 | Whole layers by contiguous range | Experts stay with their layer | Each rank stores KV for local layers | Rank 0 frontend; rank 1 stage-local handles |
| **Pipeline** | Stage A + scheduler | Stage B + sampler | Rank 0 | Rank 1 | Same as contiguous split | Same as contiguous split | Same as contiguous split, plus queued microbatches | Rank 0 global; both stage queues |
| **MoE layer-local** | First layers, routers, local-layer experts | Later layers, routers, local-layer experts, head | Rank 0 | Rank 1 | Contiguous layers | All experts attached to owned layers | Local attention-layer KV | Rank 0 global |
| **MoE expert service** | Non-expert model, routers, local experts, head | Remote expert worker | Rank 0 | Rank 0 | Non-expert model remains rank 0 | Experts partitioned \(E_A,E_B\) | All attention KV rank 0 | Rank 0; rank 1 transient buffers only |
| **Remote speculation, greedy** | Draft model + draft KV + frontend | Target + target KV + verifier | Rank 0 canonical | Rank 1 target greedy authority | Complete draft vs complete target | Local to each complete model | Draft KV rank 0; target KV rank 1 | Rank 0 external, rank 1 verification epoch |
| **Remote speculation, stochastic** | Draft proposal model/RNG | Target verifier + final sampler/RNG | Rank 0 canonical, exact mirror rank 1 | Rank 1 final; proposal RNG rank 0 | Complete draft vs complete target; optional draft replica on rank 1 only if protocol says so | Local to each model | Draft and target KV separate | Split protocol state; rank 1 owns committed target prefix |
| **Replicated decode** | Full independent replica for A sessions | Full independent replica for B sessions | Replicated | Local to assigned replica | Complete model on each node | Complete expert set on each node | Full KV only for locally assigned sessions | Assigned replica / external router |
| **Layer + pipeline hybrid** | Same as split, with queues | Same as split, with queues | Rank 0 | Rank 1 | Contiguous layers | Layer-local | Local layers and queued sessions | Rank 0 global |
| **Prefill/decode migration** | Prefill model/placement; KV source | Decode model/placement; KV destination | Rank 0 canonical | Rank 1 after handoff | Phase-capable model(s) as declared | Phase-capable as declared | Transactional source-to-destination transfer | Rank 0 during migration; rank 1 after commit |

## Ownership invariants

### Rank

There are exactly two model-execution ranks in the base topology. Additional CPU threads, RPC workers, or stream endpoints do not create independent model-state authorities unless declared.

### Tokenizer

The canonical tokenizer controls text normalization, special token IDs, chat template, and tokenization. In split modes, rank 0 owns it. In replicated mode, both ranks own byte-identical copies. Draft and target remote speculation must use compatible token IDs; “similar vocabulary” is insufficient.

### Model

Model ownership includes version, quantization, tensor layout, and execution responsibility. A weight may be:

- fully local;
- replicated;
- assigned by contiguous layer;
- tensor-sharded;
- assigned by expert.

No tensor should be silently loaded on both ranks if the memory model counts it once, or absent from both because each assumes remote ownership.

### Expert

An expert owner stores the expert weights and computes its output. The router owner decides assignments. Expert ownership does not imply KV ownership. Remote expert workers must not mutate session state beyond transient buffers.

### KV

KV ownership is indexed by session, layer, and—under TP—head shard. Every append must have one authoritative destination. Retries and cancellations need epochs or sequence numbers to prevent duplicate appends.

### Sampler

The sampler is more than an `argmax`. It includes logits bias/masks, repetition/frequency penalties, temperature, top-k/top-p/min-p, grammar constraints, stop handling, and RNG. In a sharded vocabulary, sampling may be a distributed protocol, but final emission and RNG counter advancement have one authority.

### RNG

Proposal RNG and final-output RNG are separate in stochastic speculation. Replicas own per-session RNG locally. Split modes keep final RNG with the sampler rank. Failover/replay must preserve counters or explicitly restart sampling semantics.

### Session

External session ownership covers request cancellation, timeout, output commitment, and error reporting. Stage-local KV handles are not the external authority. Unverified speculative tokens must never become externally committed state.

## State-transition rule

A stateful object can move only through a transaction:

1. source freezes or versions mutations;
2. source exports object plus schema/version/checksum;
3. destination imports and validates;
4. coordinator atomically changes ownership;
5. source releases only after commit.

This applies to KV migration, sampler/RNG failover, model hot swap, and expert-placement updates.
