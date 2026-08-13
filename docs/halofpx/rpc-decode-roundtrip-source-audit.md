# RPC decode round-trip source audit

Status: **source audit complete; no bounded RPC round-trip implementation
admitted; follow-up tracked in
[#58](https://github.com/JCFrags/HaloFPX/issues/58)**

This record tests a specific performance hypothesis: whether the model-general
dual-node generation path pays a redundant RPC completion round trip that can
be removed without changing graph ownership, scheduling, or failure semantics.
It does not. The ordinary feature-off path already sends graph execution
without waiting for a response and uses the following output read as the first
completion boundary.

The audit did find a larger authenticated-composed omission: that optional
lane explicitly disables llama graph reuse, so it rebuilds the scheduler plan
and sends the full serialized RPC graph on every admitted execution. A simple
reuse enablement is unsafe. Fresh scheduler/mutable authority changes the
authenticated transcript while current RPC recompute requires equality with
the prior transcript, and the scheduler has no retained pre-rewrite authority
plan to rehydrate. The bounded implementation therefore requires two explicit
phases recorded in issue #58; it is not a local round-trip patch.

No engine source, protocol, build default, deployment, or target state changed.
Issue [#41](https://github.com/JCFrags/HaloFPX/issues/41) remained in force, so
this audit performed no build, inference, or benchmark on either Strix Halo
machine. The source authority is exact HaloFPX commit
`3bf10a1f4afcb18dfa17c657f17d0e54c82d48fb`.

The twelve relevant RPC protocol/ABI, transport, scheduler, llama caller, and
focused-test blobs were rechecked on 2026-08-13 against rebased main
`410d0efcda2f486eb2ba50ea671eb74fa922b3b0`. Every blob ID remained
identical, so the audit remains current on that exact main commit. Exact blob
IDs are retained in the machine-readable receipt.

## Exact ordinary-RPC path

`ggml_backend_sched_compute_splits()` submits each split through
`ggml_backend_graph_compute_async()`. For an RPC split, the ordinary backend
implementation sends `GRAPH_COMPUTE` for a new graph or `GRAPH_RECOMPUTE` for
a matching nonzero graph UID and returns success without receiving a response.
The RPC backend exposes no asynchronous tensor get/copy methods or events, and
`ggml_backend_rpc_synchronize()` is explicitly a no-op.

When a later split needs an RPC-owned output, the scheduler's blocking copy
falls through to `GET_TENSOR`. The connection-local server dispatches commands
serially: it finishes `GRAPH_COMPUTE` or `GRAPH_RECOMPUTE`, sends no response,
then receives and services `GET_TENSOR`. The `GET_TENSOR` response therefore
already proves that the earlier compute completed on that ordered connection.
Sending the compute and read as one new opcode would not remove another
completion wait.

The steady reused-graph control records have these exact feature-off sizes:

| Record | Payload | Request wire | Response wire | Completion wait |
| --- | ---: | ---: | ---: | --- |
| `GRAPH_RECOMPUTE` (opcode 16) | 4 B device ordinal | 13 B | none | none |
| `GET_TENSOR` (opcode 8) | 312 B | 321 B | 8 B length + `N` output bytes | yes |

The 312-byte GET payload is a 296-byte packed `rpc_tensor`, an 8-byte offset,
and an 8-byte length. Request framing is one opcode byte plus an 8-byte payload
length. These values are ABI facts at the audited commit, not measured network
packet sizes. Other tensor updates and cross-backend copies can also occur in a
complete token; this table isolates the proposed compute/output boundary.

The relevant source anchors are:

- `ggml/src/ggml-rpc/ggml-rpc.cpp:52-114`, packed tensor and command ordinals;
- `ggml/src/ggml-rpc/ggml-rpc.cpp:215-219`, GET request layout;
- `ggml/src/ggml-rpc/ggml-rpc.cpp:2755-2793`, request/response framing;
- `ggml/src/ggml-rpc/ggml-rpc.cpp:3571-3578`, blocking GET client;
- `ggml/src/ggml-rpc/ggml-rpc.cpp:3767-3770`, no-op synchronize;
- `ggml/src/ggml-rpc/ggml-rpc.cpp:4293-4312`, send-only graph reuse/compute;
- `ggml/src/ggml-rpc/ggml-rpc.cpp:8914-8960`, serial server dispatch; and
- `ggml/src/ggml-backend.cpp:2242-2530`, serialized scheduler split execution.

## Exact authenticated-composed cost and omission

The optional composed lane is not the ordinary product path. It requires the
compile-time local-state authority and an explicitly armed execution; the only
in-tree arming caller at the audited commit is the distributed-state canary.
That statement is scoped to the llama composed lane. The RPC mutable-authority
test also arms the lower-level backend seam directly, without entering that
llama lane.
No current production or performance claim follows from this lane.

Within `llama_context::process_ubatch()`, graph reuse requires
`!composed_authority`, so every composed execution resets the graph result and
scheduler, builds and splits again, and receives new scheduler graph/split
UIDs. Every RPC split then takes authenticated full `COMPUTE`, not retained
`RECOMPUTE`.

For `N_nodes` split nodes and `N_tensors` recursively serialized tensors, the
exact legacy graph serialization is:

`G = 12 + 8*N_nodes + 296*N_tensors`

Excluding one-time HELLO/CAPS, allocation/setup, scheduler mutable traffic, and
any conditional output GET, one authenticated full-graph split costs:

| Phase | Client to server | Server to client | Blocking exchange |
| --- | ---: | ---: | ---: |
| authenticated `COMPUTE` preparation | `273 + G` B | 272 B | 1 |
| separately authenticated `EXECUTE` | 273 B | 272 B | 1 |
| total | `546 + G` B | 544 B | 2 |

The per-split total is therefore `1,090 + G` application bytes and two
required blocking exchanges. Current authenticated recompute would be 1,090
bytes with the same two exchanges; its possible benefit is removal of `G`, not
removal of either accepted authority phase. A remote output read, when the
actual placement requires one, adds `329 + B` bytes and one exchange. Output
placement is graph/model/configuration dependent, so the audit does not claim
that GET unconditionally.

The server currently retains exactly one stored graph and authenticated
lineage slot per endpoint/device. A later full compute overwrites that slot, so
multiple RPC splits mapped to the same endpoint/device cannot all recompute on
the following execution. Any reusable-split byte claim must therefore use an
explicit cardinality policy: either refuse unless there is exactly one
reusable split per endpoint/device, or negotiate a bounded multi-UID table with
defined capacity, memory ownership, eviction, invalidation, allocation-epoch,
and failure behavior.

## Retained measurements and bounded interpretation

**[MEASURED]** [P08](p08-exact-model-critical-path-profile.md) recorded 128
long-duration filtered `recvfrom` calls on the coordinator for 128 generated
tokens and 129 on the worker. Their approximately
29--31 ms medians aligned with alternating remote and local GPU work. P08 also
recorded only tens of megabytes over the 20-second monitor window and roughly
half-duty GPUs. The profile supports a serialized rank-dependency
`[INFERENCE]`; it is not a command-level trace and does not prove that every
socket call maps one-to-one to a token. The exact raw-evidence routing and
environment scope are retained in the
[P08 receipt](evidence/p08-exact-model-critical-path-profile-receipt.json).

**[MEASURED]** [P05](p05-rpc-small-command-coalescing-rejection.md) tested the
narrower control-plane mechanism directly.
Coalescing small RPC command framing removed 252 of 8,509 traced send calls
(2.96%) in its diagnostic request. Its matched generation point estimate was
`-0.0057%`, prompt processing was `-0.0461%`, and wall time was `+0.0592%`; all
intervals crossed zero and the candidate was rejected and removed. P05 does
not prove that all future command reductions are valueless, but it is direct
evidence against spending correctness complexity on one 13-byte recompute
record or a few hundred bytes of GET metadata while preserving the same wait
and output transfer. Exact build, target, raw-bundle, and rollback authority is
retained in the
[P05 receipt](evidence/p05-rpc-small-command-coalescing-rejection-receipt.json).

No new performance claim follows from this source audit.

## Why no authenticated shortcut was admitted

The optional `GGML_RPC_HALOFPX_LOCAL_STATE` authenticated graph path does wait
for a signed recompute-preparation receipt and then a separately signed execute
completion receipt. That is not an accidental duplicate ACK. L40's accepted
contract intentionally requires the client to validate the server's
preparation receipt before authorizing execution; independent adversarial
review required that two-phase boundary. The first L40 candidate was rejected
because its receipt arrived only after backend compute; the accepted
[`L40 review`](reviews/2026-07-25__l40-rpc-graph-authority__review__v01.md)
records the correction.

A fused authenticated recompute-and-execute command could save one response
turn only by changing that accepted authority contract. It would also need a
new signed capability/version, exact peer fallback before any mutation,
connection-local outcome handling, preserved mutable-admission consumption,
and a new threat review. Failure after execution but before the final receipt
is outcome-ambiguous and cannot be retried on a replacement connection: remote
pointers, stored graphs, nonces, and KV mutation are connection-local. This is
not a correctness-contained optimization for the ordinary generation task.

Simply enabling composed `can_reuse()` also fails closed:

- fresh scheduler authority remains at generation zero because graph
  split/allocation is the only path that assigns canonical pre-rewrite tensor
  IDs and structural events, while prepare requires generation one and an
  allocated scheduler;
- pass 5 rewrites consumer source edges to scheduler-generated copies, so
  traversing the retained graph later authenticates a different topology;
- root registration, census resolution, split binding, mutable admission, and
  session setup currently live only inside the rebuild branch; and
- a fresh composed execution derives a fresh transcript from its nonce,
  execution sequence, scheduler roots, and split mapping, while server
  recompute currently requires the request transcript to equal the stored
  prior transcript byte-for-byte.

Relaxing the transcript check would permit stale/cross-attempt authority.
Reusing old authority objects would reuse secrets, admissions, receipts, or
socket state. Both are P0 failures and were rejected.

## Safe two-phase follow-up

Issue [#58](https://github.com/JCFrags/HaloFPX/issues/58) records the bounded
implementation:

1. **Retained scheduler substrate, default off.** Capture a scheduler-owned,
   secret-free canonical plan before source-edge rewriting. Rehydrate fresh
   L42 authority over its still-live exact split/copy/allocation mapping,
   freshly resolve all L44 storage/socket/allocation authority, and explicitly
   force full authenticated `COMPUTE` preparation followed by the separate
   `EXECUTE`. Invalidate on every reset, reserve, reallocation, backend,
   callback, copy-slot, buffer, connection, allocation-epoch, graph, or
   ambiguous-failure change. This can save local rebuild/split/allocation work
   only; it removes no RPC command, wait, or graph byte.
2. **Versioned prior-lineage recompute.** After phase 1 qualifies, add an exact
   capability and record that binds prior accepted UID/digest/transcript to
   the fresh attempt/admission/transcript. Preserve preparation receipt,
   client validation, and separate execute. Gate to exactly one reusable split
   per endpoint/device, or first add a capability-gated bounded multi-UID graph
   table with explicit lifetime rules. This is the first phase that can remove
   `G` graph bytes for each split admitted by that exact cardinality policy.

Both phases require exact attempts/hits/misses/invalidation reasons, graph
node/tensor/byte counters, authenticated operation counts, output transfer
counts, reusable-split/cardinality refusal and graph-table counters,
failure-phase/fencing counters, CPU/fake-RPC/loopback tests including a
same-endpoint/device multi-split case, feature-off parity, and matched Strix
Halo evidence after issue #41 permits a maintenance window.

## Ranked next work

1. **[RECOMMENDATION] Implement issue #58 in two phases.** First retain and
   rehydrate exact scheduler authority while forcing full authenticated
   compute. Then design and qualify versioned prior-lineage recompute to omit
   `G` under an exact endpoint/device cardinality policy. No phase is promoted
   from source presence.
2. **[RECOMMENDATION] Model-general rank-parallel fork/join execution.** Build
   an explicit graph dependency plan that submits only proven-independent
   rank-owned branches concurrently and joins at a typed boundary. The retained
   half-duty signal makes this the only candidate here with scale comparable to
   the observed approximately 30 ms remote/local phases. It is an architecture
   slice, not an RPC framing patch. Single-node execution must retain the
   current graph and failures must cancel the whole join without accepting
   partial KV/output state.
3. **[RECOMMENDATION] Negotiated asynchronous completion and copy events for
   prompt processing.** RPC currently exposes no async tensor copy or events,
   disabling the scheduler's existing copy pipeline. This can reduce prompt
   stalls where independent microbatch work exists, but it does not make a
   strictly dependent decode split parallel by itself.
4. **[OPEN] Device-visible activation exchange.** Profile exact activation
   sizes and host/device copies before designing a negotiated transport. P08
   rejects aggregate bandwidth saturation as the current explanation; copy
   removal remains plausible but unquantified.
5. **Reject: fused legacy recompute+GET or compact GET handles.** These
   preserve the only completion wait and output bytes while saving at most the
   13-byte recompute frame and/or most of the 296-byte tensor descriptor. P05
   supplies a stronger negative control-plane result than this proposal has a
   positive mechanism.
6. **Reject: authenticated recompute+execute fusion.** Do not silently weaken
   L40's client-validated preparation boundary. Prior-lineage recompute must
   preserve both authenticated phases.

The machine-readable audit receipt is
[`evidence/rpc-decode-roundtrip-source-audit-3bf10a1f.json`](evidence/rpc-decode-roundtrip-source-audit-3bf10a1f.json).
