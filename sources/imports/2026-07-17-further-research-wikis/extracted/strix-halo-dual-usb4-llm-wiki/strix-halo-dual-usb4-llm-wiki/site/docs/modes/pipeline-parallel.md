---
title: Pipeline parallelism
status: conditional throughput mode
---

# Pipeline parallelism

![Two-stage pipeline](../../diagrams/svg/pipeline.svg)

Pipeline parallelism uses the same contiguous ownership as a layer split and changes the schedule: multiple independent microbatches occupy different stages concurrently. GPipe established batch-splitting across sequential layer partitions as a pipeline technique. [GPipe](https://arxiv.org/abs/1811.06965).

## Ownership

Ownership is identical to [contiguous layer splitting](contiguous-layer-split.md): rank 0 owns tokenizer, embeddings, first layers, and first-stage KV; rank 1 owns later layers, head, sampler, final RNG, and second-stage KV. The difference is that both ranks maintain bounded queues indexed by session and microbatch.

Machine-readable definition: [`placements/pipeline-parallel-2.yaml`](../../placements/pipeline-parallel-2.yaml).

## Forward pipeline model

For each microbatch, let:

- \(c_A\): measured rank-0 stage compute;
- \(x\): measured boundary transfer stage;
- \(c_B\): measured rank-1 stage compute.

If compute and transport use separately overlappable resources:

\[
s=\max(c_A,x,c_B),
\]

\[
T(M)=c_A+x+c_B+(M-1)s.
\]

If communication blocks compute on its host/GPU path:

\[
s=\max(c_A,c_B)+x.
\]

Do not decide overlap from API names such as “asynchronous.” Confirm with a timeline trace.

## Prefill

Prompts or prompt chunks from independent requests form microbatches. Total boundary payload stays:

\[
V_{total}=Nhb_a,
\]

but \(M\) messages add \(M\ell\) and staging buffers. A long prompt can be chunked, but chunks from the same prompt have KV/data dependencies in the stage runtime; verify the backend's chunked-prefill schedule rather than assuming complete independence.

## Decode

One sequence cannot fill both stages with successive tokens because token \(t+1\) needs token \(t\)'s sampler result. Independent sequences can interleave:

1. rank 0 processes sequence A token \(t\);
2. while rank 1 processes A, rank 0 processes sequence B;
3. sampler results return independently; each sequence advances only after its own result.

The system-level throughput unit is a scheduling round over multiple sequences, not one sequence's serial token chain.

## Minimum concurrency

Compare pipeline makespan with serial stage compute:

\[
T_{serial}=M(c_A+c_B).
\]

A strict break-even requires \(s<c_A+c_B\) and:

\[
M>\frac{c_A+x+c_B-s}{c_A+c_B-s}.
\]

The calculator returns the smallest integer \(M\). This is necessary, not sufficient: queueing, scheduler overhead, KV paging, and tail latency can erase the gain.

## Synchronization and buffers

For \(M\) microbatches:

- \(M\) activation-boundary messages;
- decode token feedback per session/microbatch;
- bounded producer/consumer synchronization;
- in-flight activation memory at least proportional to queue depth;
- cancellation propagation and stale-message rejection.

Every message must carry session ID, token range, pipeline epoch, dtype/shape, and an ordering sequence. A retry must not append duplicate KV.

## Stage balance

Balance the measured service interval, not FLOP estimates. On a unified-memory APU, a stage's time can be affected by weight residency, KV length, CPU transport work, dequantization, and concurrent network copies. Profile prefill and decode separately; the best cut can differ.

A fixed cut serving both phases may choose:

- a compromise cut;
- phase-specific scheduling without changing ownership;
- separate queues/SLOs;
- no pipeline for low-concurrency decode.

Changing the cut between phases implies weight/KV redistribution and is a different hybrid.

## Feasibility gates

- Contiguous split memory and correctness gates pass.
- At least the calculated minimum independent microbatches are available under the real offered load.
- Measured \(s<c_A+c_B\).
- In-flight boundary and KV buffers fit with safety margin.
- Tail latency and queueing remain inside the objective.
- The transport/runtime overlap is visible in traces.
- Single-request latency is not the claimed benefit.

## Disposition

Pipeline scheduling is **conditional** and primarily a throughput optimization layered on a capacity split. It is not a mechanism for removing one sequence's autoregressive dependency.
