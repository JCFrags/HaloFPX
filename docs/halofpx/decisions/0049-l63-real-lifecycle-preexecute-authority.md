# ADR-0049: real-lifecycle pre-execute authority

**Status:** accepted design; L63 implementation not promoted

## Decision

L63 will instrument the real L42/L44/RPC lifecycle. It will not infer a
pre-execute result from verifier fixtures or from a successfully created L44
session.

One execution attempt owns a bounded authenticated recorder before L44 begin.
The recorder is identified by an opaque value handle and binds the attempt
nonce, recorder generation, execution sequence, parent graph UID, exact
scheduler split UID/backend/ordinal, RPC connection epoch, and server
allocation-topology epoch. The feature is negotiated and remains absent from
the wire and runtime when disabled.

The scheduler exposes a non-consuming, handle-bound prepared-admission value.
It is available only after the real split has been prepared and contains the
immutable prepared root, split-mapping root, parent UID, execution sequence,
attempt identity, scheduler session/generation, and split count. L44 begin
requires this value; the older generic admission snapshot is not authority for
L63.

Each accepted RPC connection has a connection epoch owned by that connection,
not derived from a graph, server, or attempt nonce. Each server connection has
an allocation-topology epoch incremented after successful allocation or free.
The feature-negotiated L44 capability response returns both. All subsequent
L44 and graph-execution authority binds them and refuses stale values.

The recorder directly receives closed events from L44
begin/register/exclude/prepare/commit/abort and from the real RPC graph-compute
decision path. Events use canonical little-endian encoding, a distinct HMAC
domain, monotonic event sequence, hard event/byte limits, and an exact terminal
grammar/count. Records are attempt-scoped; concurrent attempts cannot share a
counter, chain, or terminal state. A refusal before L44 session creation is
therefore retained.

Transport is recorded as observed stages:

`not_attempted`, `serialize_failed`, send opcode/header/body
started/completed, receive header/body started/completed, `eof`,
`syscall_error`, and `decode_refusal`.

A completed send is recorded only after the exact byte count completes.
GRAPH_AUTH_COMPUTE, GRAPH_AUTH_RECOMPUTE, and GRAPH_AUTH_EXECUTE use the same
observer. The record contains no payload bytes.

## Source authority

- Scheduler preparation and immutable split/root access:
  `ggml/src/ggml-backend.cpp`, `ggml_backend_sched_authority_prepare` and the
  adjacent split/root accessors.
- Real caller order:
  `src/llama-context.cpp`, scheduler preparation, RPC split binding, L44
  begin/register/exclude/prepare, scheduler compute, and L44 commit/finalize.
- L44 lifecycle and RPC decision:
  `ggml/src/ggml-rpc/ggml-rpc.cpp`,
  `ggml_backend_rpc_halofpx_mutable_*` and
  `ggml_backend_rpc_graph_compute`.
- Transport byte/EOF/errno authority:
  `ggml/src/ggml-rpc/transport.cpp`,
  `send_data_observed` and `recv_data_observed`.

## Refusal policy

Unknown, duplicate, reordered, missing, stale, cross-attempt,
cross-connection, cross-allocation-epoch, cross-parent, cross-split, or
post-abort events fail closed. Unknown lifecycle or graph-compute branches
also fail closed. A terminal abort is emitted once; cleanup may be idempotent
but cannot append authority after termination.

Feature-off performs no recorder allocation, traversal, synchronization,
hashing, logging, or wire change.

## Qualification gate

Stories15M remains prohibited until:

1. focused tests exercise the actual L44 and graph-compute call sites;
2. one real two-host no-model composed graph reaches authenticated execute;
3. reachable L44 and pre-execute refusals are harvested through L61;
4. pre-send, partial-send, response-header/body EOF, syscall, and decode
   failures have honest transport records;
5. concurrent attempts remain isolated; and
6. independent review accepts source and evidence.

If this gate fails, L63 closes without a stories run or semantic correction.
