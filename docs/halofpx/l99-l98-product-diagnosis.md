# L99 L98 product diagnosis

Status: **READ-ONLY DIAGNOSIS COMPLETE; NO PRODUCT CORRECTION AUTHORIZED**

The canonical machine-readable comparison is
[`l98-canonical-diff.json`](evidence/l99-offline/l98-canonical-diff.json).
It binds the retained inputs by SHA-256 and distinguishes authenticated
represented equality from live-state completeness.

## Earliest retained divergence

Capture and restore agree through the represented coordinator control bytes,
coordinator-local bytes, 64-component worker manifest, token boundary, KV
logical geometry, attention views, graph input digest, backend placement, and
4,719-node assignment digest. Five independently harvested server authorities
also prove four capture executions and the restore execution reached
authenticated server terminal success.

The first retained unequal value is therefore the post-execution logits:
capture `8564aef9...c754` / token `21549` / suffix ` alpha`, versus restore
`7a8807f4...36cd` / token `9283` / suffix `计划`. The evidence does **not**
show an earlier represented-byte difference. It also does not prove that the
represented byte sets cover every byte or runtime state read by the replay
graph.

## Exact source read-set boundary

The canary synchronizes before hashing/printing semantic provenance
([test-halofpx-distributed-state-canary.cpp](../../tests/test-halofpx-distributed-state-canary.cpp)),
while KV replay resets and allocates the scheduler graph before compute
([llama-kv-cache.cpp](../../src/llama-kv-cache.cpp)). The scheduler then copies
split inputs, coordinates backend events/synchronization, and dispatches each
backend graph asynchronously
([ggml-backend.cpp](../../ggml/src/ggml-backend.cpp)). RPC reconstructs remote
tensors/buffers and executes its split
([ggml-rpc.cpp](../../ggml/src/ggml-rpc/ggml-rpc.cpp)).

Consequently the final ROCm logits read:

1. serialized KV logical bytes and coordinator metadata (cell/head/sequence);
2. KV view strides, offsets, quantized block layout, and any padding/tail bytes
   kernels can address;
3. scheduler-created cross-backend copies and transient allocation contents;
4. RPC-reconstructed storage/view aliases and their completion visibility;
5. model weights plus stateless attention/MoE router/expert kernels.

The model's router/expert state is regenerated activation state, not an
independently persistent recurrent state in the inspected path. It therefore
does not outrank KV/storage/copy visibility without evidence of an unequal
weight or graph input; those are equal or authenticated by the retained
records.

## Ranked source-supported candidates

1. **An unrepresented replay input** (joint highest). L31/L33 established
   primary token divergence, while the successful L34/L35 Stories fixture
   showed that the mechanism can reproduce logits in a smaller topology.
   L35 explicitly left token/position/mask bindings, attention-index contents,
   and per-node assignment unresolved. L98 closes node assignment and the
   one-token graph input, but not every auxiliary input value.
2. **Logical serialization omits kernel-readable storage bytes** (joint
   highest). L35/L37-L44 added KV geometry, graph-input, placement, lifecycle,
   and authenticated execution authority, but did not authenticate allocation
   padding, quantized tails, or stride gaps.
3. **Restore visibility/order for RPC or scheduler copies.** The source uses
   asynchronous backend operations and explicit event/synchronization seams.
   Server terminal success proves execution, not that every restored write was
   visible to every subsequent split in the same way as capture residency.
4. **RPC reconstruction/alias or transient allocator state.** Authenticated
   geometry is equal, but reconstructed buffer/view identity and transient copy
   contents are not byte-attested.
5. **Architecture-specific transient computation.** Possible only downstream
   of the above read-set; retained equal weights/input/topology and repeatable
   token pair make an independent router/expert-state hypothesis weaker.

The evidence cannot distinguish candidates 1-4 because it has no per-layer
pre-attention input/output digest or full physical-storage-range digest at the
capture/apply boundary.

## Smallest next discriminator

First use an offline/no-model range-coverage audit: for each of the 124 retained
KV tensors, derive every byte interval addressable by its type, dimensions,
strides, view offset, and backend buffer allocation; compare that union with
the exact capture/apply component ranges. This needs no model run and will
either prove a serialization hole (including padding/tails) or eliminate
candidate 2.

Only if interval coverage is exact should a future primary attempt add the
minimum authenticated instrumentation: one digest per layer at the
post-KV-restore/pre-attention input and post-attention output boundaries, after
explicit backend synchronization, on both RPC and ROCm halves. Stop at the
first unequal layer; do not add a broad tensor trace.

## Response-verifier P2

The verifier now treats the retained files as five exact authenticated
productions keyed by attempt, connection epoch, split UID, execution sequence,
backend ordinal, and opcode. It rejects interleaving, replay, gaps, cross-pair
identity changes, incomplete pairs, and revisited groups. L98 replays as four
capture pairs plus one restore pair. This closes cross-attempt mixing, but does
not prove zero legacy state-page GET/SET: L98 lacks an authenticated bounded
transport window, and ordinary model-load tensor traffic cannot safely be
excluded by an unbounded absence search.
