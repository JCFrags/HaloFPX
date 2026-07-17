---
title: Implementation notes
status: design guidance, backend-neutral
---

# Implementation notes

The cost model is runtime-neutral. This page identifies the minimum implementation surface needed to turn a placement into a correct two-rank system.

## Process and rank model

Use one model-execution authority per node:

```text
rank 0 -> node A -> transport endpoints path 1/path 2
rank 1 -> node B -> transport endpoints path 1/path 2
```

Worker threads, command queues, or RPC handlers remain subordinate to the rank that owns the associated model/KV/sampler state. Assign stable rank IDs independently of transient network-interface enumeration.

## Control plane

The control plane should negotiate:

- protocol and placement schema versions;
- model/checkpoint/quantization hashes;
- tokenizer and special-token hashes;
- supported dtypes, tensor layouts, compression, and endianness;
- context, batch, and buffer limits;
- session and KV epoch rules;
- sampler/RNG capabilities;
- path health and aggregation policy.

Reject incompatible peers before allocating model-path buffers.

## Data plane

Implement explicit message types rather than an untyped RPC blob:

| Message class | Examples | State effect |
|---|---|---|
| Immutable tensor | layer boundary, collective operand | None until operation completes |
| Expert request | hidden state + expert ID | Transient remote compute only |
| Expert response | expert output + route metadata | Combined by router owner |
| Speculative proposal | candidate IDs/probability protocol | Uncommitted |
| Sample/commit | accepted length and token(s) | Advances authoritative prefix and RNG |
| KV transaction | export chunks, manifest, commit/abort | Changes owner only after commit |
| Control | cancel, health, backpressure | Protocol/session state |

## Tensor layout

For every tensor crossing the link, define:

\[
(dtype,\ shape,\ logical\ axes,\ physical\ strides,\ quantization,\ byte\ order).
\]

Do not count an activation as \(Nhb_a\) and then transmit padded, converted, or framed bytes without recording the actual wire volume. Keep both **logical bytes** and **wire bytes** in traces.

## Buffering and copies

A robust implementation records the path:

```text
producer allocation
 -> optional device/host staging
 -> serialization/framing
 -> transport send buffer
 -> receive/reorder buffer
 -> optional host/device copy
 -> consumer-ready allocation
```

The cost equation's \(\ell\) and \(B\) may absorb this path only when calibrated with the same primitive and size range. Otherwise, model copies separately.

## Link scheduler

Provide three explicit modes:

1. `single_link`: choose the measured lower-completion path by message class.
2. `session_hash`: pin a session/flow to one path and avoid packet-level reorder.
3. `validated_striping`: partition large tensors using calibrated path weights and bounded reassembly.

A health failure must downgrade deterministically. Changing from striped to single-link mid-operation requires an operation epoch; do not concatenate duplicate/retried stripes.

## Mode implementation notes

### Tensor parallelism

- Shard every compatible linear/embedding/head tensor consistently.
- Trace two intended forward reductions per transformer layer for the Megatron-style layout; alternate architectures require their own count.
- Define attention/KV sharding when \(H_{kv}\) is not divisible by two. Replication may change memory and communication.
- Define vocabulary-parallel sampling. A hidden full-logit gather can dominate decode and must appear in both ownership and cost models.
- Keep residual/norm semantics consistent with the chosen graph.

### Contiguous layer split

- Rank 0 owns embeddings and layers before cut \(k\); rank 1 owns remaining layers, final norm/head, and sampler.
- Allocate KV only for local layers.
- Carry position IDs, attention mask/window metadata, sequence lengths, and cache handles with the boundary.
- Return committed token IDs to rank 0 before the next decode embedding step, unless embeddings are deliberately replicated and that alternative is costed.

### Pipeline

- Reuse contiguous ownership; add bounded queues and microbatch IDs.
- Schedule only independent work that is safe to overlap. Autoregressive tokens within one sequence remain dependent.
- Apply backpressure before boundary buffers consume the memory safety margin.
- Preserve per-session ordering across the two stages.

### MoE expert service

- Router owner emits `(session, position, layer, expert, route_weight, vector)` records.
- Expert workers are stateless functions of the declared model revision and input.
- Combine owner restores original token/route order and weights outputs correctly.
- Batch remote assignments by layer only within latency and memory bounds.

### Remote speculation

- Keep draft and target KV separate.
- Mark proposed suffixes uncommitted.
- The target rank owns accepted length, correction/next token, final sampler, and final RNG.
- Stochastic mode must specify how proposal distribution \(q\) is reconstructed for residual sampling; candidate IDs alone are not a generic exact protocol.
- Stop, grammar, penalty, and tool-call rules need a single authoritative application point.

### Replicated decode

- Each rank is a complete independent replica for its assigned sessions.
- An external or rank-0 router can route new sessions, but should not migrate live KV by default.
- Session affinity and admission control should account for context length, not request count alone.

### KV migration

- Export a versioned manifest before chunks.
- Freeze or version source appends.
- Validate target layout, checksums, position, rope/scaling metadata, and model revision.
- Commit ownership atomically; release source only after acknowledgment.

## Runtime avenues and evidence boundary

A runtime may expose point-to-point RPC, layer offload, collectives, or custom kernels. AMD's published llama.cpp RPC cluster demonstrates one distributed-runtime avenue on Ryzen AI Max+ hardware. It does not imply that dual USB4, TP collectives, exact remote speculation, or expert service are supported without additional work. Treat every primitive as a gate in [`feasibility/gates.md`](feasibility/gates.md).

## Observability

Emit per operation:

```text
session/request/operation ID
mode and placement revision
rank and message class
logical bytes and wire bytes
path/chunk map
queue, serialization, send, receive, and consumer-ready timestamps
compute begin/end
epoch and retry count
memory high-water marks
correctness/checksum status
```

This separation is necessary to distinguish network, copies, queueing, synchronization, and compute.

## Failure policy

A production policy should state whether a path, rank, or request failure causes:

- retry of an idempotent transfer;
- downgrade to one link;
- restart from the last committed token;
- abort of the request;
- failover to a full replica.

Failover that changes tokenizer, sampler, RNG counters, quantization, or logits processing is a semantic change, not transparent recovery.
