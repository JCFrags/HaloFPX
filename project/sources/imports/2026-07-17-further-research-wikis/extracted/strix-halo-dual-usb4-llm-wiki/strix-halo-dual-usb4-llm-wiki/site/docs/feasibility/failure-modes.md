---
title: Failure modes
status: operational analysis
---

# Failure modes

## Link-level failures

### One USB4 path drops

- **Replicated decode:** sessions on either node can continue if request routing does not depend on the failed path; no automatic in-flight session failover without KV reconstruction.
- **Single-link layer split/TP/expert service/speculation:** affected sessions stall or fail.
- **Striped bulk flow:** the whole message must fail or resume through a protocol with chunk checksums and epochs; silently accepting partial data is invalid.
- **Control-plus-bulk:** preserve cancellation/error reporting on the surviving control path where possible.

### Reordering or duplicate delivery

Bulk chunk headers need message ID, session ID, model epoch, tensor identity, byte range, checksum, and sequence number. Duplicate activation or expert output application can corrupt KV or hidden state without an obvious transport error.

### Bandwidth collapse under concurrent paths

Fail the striping gate and revert to single-link or session-hash policy. Do not continue using \(B_1+B_2\) in cost models.

## Rank/process failures

### Tensor parallel

Every token depends on both ranks and both KV shards. A rank failure invalidates the active session unless full state is checkpointed elsewhere. There is no partial continuation.

### Contiguous split and pipeline

Each rank holds unique layer weights/KV. Failure invalidates sessions. Recovery options are:

- restart and re-prefill from committed token history;
- restore a compatible checkpoint of both stage KVs;
- abandon the request.

### Remote expert service

A remote expert failure can make selected routes unavailable. Substituting a local or different expert changes the model and is not a transparent recovery. Fail the request or use an explicitly designed replicated-expert placement counted in memory.

### Remote speculation

Unverified candidates are disposable. If the draft rank fails, the target may continue baseline decode if the frontend/session can move to rank 1 and target KV is authoritative. If the target fails, no candidate can be committed. This asymmetry should be reflected in session authority.

### Replicated decode

A healthy replica can accept new requests immediately. Existing sessions on the failed replica require re-prefill from committed token history or a KV checkpoint. “Two replicas” is not synonymous with transparent stateful failover.

## State-version failures

### Model mismatch

Different checkpoint revisions, quantization metadata, tokenizer files, rope scaling, or chat templates can produce silent divergence. Handshake every rank with immutable hashes before accepting requests.

### Placement epoch mismatch

Expert maps, layer cuts, and tensor shard layouts must have a shared epoch. Never change placement mid-request unless the state-transition protocol explicitly migrates model/KV ownership.

### RNG divergence

Retrying a sampling message can advance RNG twice. Carry request/step/round counters and make sampler operations idempotent. Separate draft proposal RNG from final target RNG.

### KV duplicate or gap

Every KV append should include session, layer/head shard, token position, and epoch. Reject duplicate positions with different data and out-of-order gaps. Retrying a boundary message must not append twice.

## Resource failures

### Memory fragmentation/OOM

Model load success does not prove peak feasibility. Pipeline queues, long prompts, speculative blocks, expert buffers, and concurrent KV can push the runtime over the edge. Enforce admission control from measured per-session KV and buffer budgets.

### Queue runaway

A faster producer can fill activation or expert queues behind a slower consumer. Use bounded queues and backpressure. Include queue occupancy in health checks; do not trade unbounded memory for average throughput.

### Host-copy bottleneck

USB4 networking may introduce CPU and memory copies. A nominally asynchronous transport can serialize with GPU/APU execution or saturate shared memory bandwidth. Profile copy engines, CPU use, page faults, and compute overlap.

### Thermal throttling

A short benchmark can select a mode that fails in steady state. Use sustained runs and compare phase timelines before and after thermal equilibrium.

## Correctness failures by mode

| Failure | Affected modes | Detection |
|---|---|---|
| Full-logit gather omitted | TP with vocab sharding | Compare sampler probabilities/tokens against one-rank baseline. |
| Token IDs incompatible | Remote speculation / split tokenizer | Hash tokenizer artifacts and run exhaustive special-token tests. |
| Proposal q data insufficient | Exact stochastic speculation | Formal protocol review plus seeded/statistical regression. |
| Expert output combined twice | Expert service | Sequence-numbered assignments and deterministic layer tests. |
| KV assigned to expert worker | Expert service | Placement validation; worker API must be stateless. |
| Next token begins before sampler result | Pipeline | Per-session dependency trace. |
| Layer cut changes without KV migration | Hybrid | Placement epoch and KV-owner audit. |
| Aggregate bandwidth assumed | All dual-link bulk modes | Simultaneous-link benchmark gate. |

## Failure policy requirement

Every deployment should select one of three policies per mode:

- **fail request immediately**;
- **restart/re-prefill from committed token history**;
- **restore from a versioned state checkpoint**.

An unspecified recovery policy is a NO-GO for production use, even when performance gates pass.
