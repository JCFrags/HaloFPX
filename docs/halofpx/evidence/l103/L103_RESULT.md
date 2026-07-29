# L103 terminal result

Status: **SEMANTIC BLOCKER — NOT IMPLEMENTED**.

No source, build, model, host, or production action was taken. The read-only
fit check proves that the reachable exact-key server cache and the retained
rank-local distributed state protocol do not share an authoritative typed
topology/ownership object.

## Exact boundary

The current server lane is explicitly coordinator-monolithic:

- `tools/server/server-context.cpp:1188-1191` assigns global plan, rank
  ownership, and rank placement the same externally supplied compatibility
  component and hard-codes topology epoch 1.
- `tools/server/server-context.cpp:1398-1399` builds only
  `world_size=1,rank=0` transformer profiles.
- `tools/server/server-context.cpp:4933-4940` repeats the same digest,
  epoch-1, world-1/rank-0 construction for the exact-session key.
- `tools/server/halofpx-context-store-state-transformer-v1.cpp:103-104`
  admits only world 1/rank 0. Its production API uses monolithic
  `LLAMA_STATE_SEQ_FLAGS_NONE` serialization.
- `tools/server/halofpx-context-store-v1-transformer-codec.cpp:20` identifies
  a fixed world-1/rank-0 profile. Lines 491-492 require world size 1 and the
  fixed frame roster; line 513 requires the token and state descriptors to be
  rank 0.

The distributed state lane has a different, stronger input contract:

- `ggml/include/ggml-rpc.h:41-66` binds logical rank, world size, key
  generation, channel binding, model, compatibility, plan, topology,
  placement, checkpoint, token prefix, component manifest, and attempt.
- `ggml/src/ggml-rpc/ggml-rpc.cpp:8227-8231` validates key generation,
  rank/world, and channel binding.
- `include/llama.h:994-1012` exposes capture, stage, commit, and abort only
  against a typed `llama_state_seq_storage`, complete RPC identity, and
  control key.

The server exact-key lane has no typed API that supplies the live worker
identity, key generation/channel, endpoint/device/socket/allocation identity,
component manifest, or worker object custody. Its lookup occurs through the
world-1 snapshot API. Replacing the hard-coded rank values, copying opaque
configuration, or parsing execution-result text would not establish
independent live topology authority.

Manifest v1 can structurally carry an ordered rank roster
(`tools/server/halofpx-context-store-format.cpp:274-306`), but the existing
transformer producer and codec do not emit or validate a distributed roster
or an authenticated external worker-state object.

## Safety conclusion

Direct composition inside the stated L103 boundary would require accepting
one of two invalid designs:

1. label the monolithic coordinator state as distributed, leaving worker state
   unrepresented; or
2. derive cache identity from duplicated/opaque configuration rather than the
   live scheduler/RPC allocation and ownership authority.

Either can publish partial state or select a cache entry under fictitious
topology. This conflicts with corruption-as-miss/recompute, exact-key
ownership, and the instruction not to add another authority/protocol layer.

## Smallest product-composable design

Reuse the existing manifest-v1 outer container and rank-local wire protocol,
but authorize one shared internal product seam:

1. A scheduler/RPC-produced immutable distributed-checkpoint topology
   authority containing the ordered rank roster; distinct plan, ownership,
   placement, topology digests and epoch; endpoint/device/session identity;
   nonsecret key-generation/channel identity; component-manifest digest; and
   authenticated worker-object/custody digest.
2. A separately named, default-off distributed transformer profile/codec.
   Its ordered payload represents coordinator control/local state and the
   authenticated external worker object. The existing world-1 codec remains
   byte/behavior unchanged.
3. A typed llama/server transaction bridge:
   capture local storage and authenticated worker object, then on a fresh
   residency perform control prepare, live-topology reconciliation,
   authenticated worker stage, local staging, remote commit, and final apply.
   Any failure aborts and recomputes from a clean context.

This is a product implementation seam, not a harness, but it is necessarily a
new shared authority/profile contract. Lead authority is required because
L103 explicitly prohibited creating such a layer.
