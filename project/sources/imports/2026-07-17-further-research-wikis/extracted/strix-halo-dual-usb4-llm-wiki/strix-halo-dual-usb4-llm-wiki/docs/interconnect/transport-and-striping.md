---
title: Transport and striping
status: implementation-neutral cost model
---

# Transport and striping design

The physical topology supplies paths; the runtime must supply framing, tensor serialization, flow control, retries, ordering, and—when used—striping.

## Base message model

For a payload of \(V\) bytes on one path:

\[
T_{msg}=\ell+\frac{V}{B}.
\]

For \(n\) serialized messages with total payload \(V\):

\[
T_{serial}=n\ell+\frac{V}{B}.
\]

For bidirectional request/response traffic:

\[
T_{round}=\ell_{AB}+\frac{V_{AB}}{B_{AB}}+T_{remote}+
\ell_{BA}+\frac{V_{BA}}{B_{BA}}.
\]

Every mode-specific formula in this wiki reduces to one of these forms plus compute, queueing, and synchronization.

## Optimal static split across unequal paths

Let \(x_1+x_2=V\), where \(x_j\) is the payload sent on path \(j\). With concurrent sends and reassembly cost \(\delta_{reasm}\):

\[
T_{stripe}=\max\left(\ell_1+\frac{x_1}{B_1},
\ell_2+\frac{x_2}{B_2}\right)+\delta_{reasm}.
\]

When both paths are worth activating, equalize their finish times. Define:

\[
t^*=\frac{V+B_1\ell_1+B_2\ell_2}{B_1+B_2},
\]

\[
x_1=B_1(t^*-\ell_1),\qquad x_2=B_2(t^*-\ell_2).
\]

This solution is valid only when \(x_1,x_2\ge0\). If one is negative, the lower-completion-time solution uses only the other path. A runtime should calibrate by message-size band rather than always splitting 50/50.

## Symmetric-path activation threshold

For two identical measured paths \((B,\ell)\), equal striping has:

\[
T_2=\ell+\frac{V}{2B}+\delta_{stripe}
\]

versus:

\[
T_1=\ell+\frac{V}{B}.
\]

Striping improves completion time only when:

\[
V>2B\delta_{stripe}.
\]

Here \(\delta_{stripe}\) includes the extra dispatcher, copies, descriptors, reordering, and reassembly—not just a constant chosen for the model. It is a **MEASURED INPUT REQUIRED**.

## Chunking and tail completion

If a payload is striped in chunks of \(c\) bytes, the chunk count is approximately \(n=\lceil V/c\rceil\). Excessively small chunks increase framing and scheduler costs; excessively large chunks prevent load balancing when paths differ. Record:

- chunk size and count;
- per-path bytes and completion timestamps;
- reorder-buffer peak;
- final-chunk tail;
- cancellation behavior.

For critical-path modeling, use the slowest finishing path, not \(V/(B_1+B_2)\) in isolation.

## Recommended channel separation

A practical two-link implementation can expose logical channels:

1. **Control:** request headers, token IDs, accepted length, epochs, cancellation, health.
2. **Bulk:** activations, collective tensors, expert dispatch/output, KV or probability blocks.
3. **Telemetry:** traces and counters, kept off the critical path where possible.

`control_plus_bulk` does not necessarily mean one physical link is permanently idle for bulk. It means control latency must not be hidden inside a large queued tensor transfer.

## Framing requirements

Every cross-node model message should include at least:

```text
protocol_version
session_id
request_id / speculative_round / microbatch_id
model_revision and placement_revision
phase and tensor_role
dtype, shape, strides/layout
sequence start and token count
source rank, destination rank
KV/session epoch
payload length and checksum
stripe index/count when applicable
```

The receiver must reject stale epochs, duplicate non-idempotent operations, incompatible tensor layouts, and over-sized allocations before mutating authoritative state.

## Retries and idempotency

- **Immutable transfers** such as boundary activations may be retried when keyed by a unique operation ID.
- **KV append** is not blindly retryable; it requires an epoch and expected sequence position.
- **Sampling** is not blindly retryable; RNG counter advancement and output commitment must be transactional.
- **Expert service** requests should be pure functions of input tensor, expert ID, and model revision.
- **Speculative tokens** remain uncommitted until the target owner acknowledges them.

## Collective implementation for two ranks

For a p=2 all-reduce over an \(S\)-byte tensor, per-rank sent bytes are approximately \(S\) for common direct-exchange or ring decompositions, but the number of latency-bearing phases can differ. Represent the measured implementation with \(m_{AR}\):

\[
T_{AR,p=2}=m_{AR}\ell+\frac{S}{B}.
\]

Do not set \(m_{AR}=1\) or \(2\) from an algorithm name alone. Trace the actual backend, including staging copies and barriers.

## Ordering and dual-link correctness

Validated striping requires:

- deterministic reconstruction of tensor byte ranges;
- bounded memory under out-of-order completion;
- no partial visibility before checksum/shape validation;
- cancellation that drains or invalidates all stripes;
- timeout policy that does not commit half a collective or KV transfer;
- failure fallback that does not silently change sampler/RNG semantics.

## Transport acceptance test

A transport is ready for mode testing only after it passes:

- per-path and simultaneous size sweeps;
- bidirectional and full-duplex sweeps;
- sustained error/retry test;
- forced disconnect and process-kill tests;
- tensor integrity and stale-epoch tests;
- queue/backpressure test at maximum planned concurrency;
- repeatability after reboot and cable/port enumeration changes.
