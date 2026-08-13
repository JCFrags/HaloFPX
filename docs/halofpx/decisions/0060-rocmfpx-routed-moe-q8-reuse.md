# ADR-0060: bounded ROCmFPX routed-MoE Q8_1 preparation reuse

**Status:** proposed

## Context

At exact source base `9bfccf25d43af0c446df591035e9cdac0b74d6c0`,
two adjacent routed-MoE gate/up `MUL_MAT_ID` prompt MMQs with the same
activation and IDs independently launch the ID mapping helper and independently
gather/convert that activation to Q8_1. The existing graph fusion already
recognizes the no-bias pair, while merged dense-FFN reuse excludes
`MUL_MAT_ID`.

Removing duplicated local preparation is a model-general MoE opportunity. It
is not specific to MiniMax or any tensor name. Issue #41 blocks target access,
so no runtime reachability, correctness, or performance conclusion is
available.

## Decision

Add a default-off HIP option that reuses preparation only when a local
`gfx1151` backend proves all of the following:

1. The nodes are the existing adjacent no-bias routed gate/up/GLU group.
2. Both operations share the exact F32 activation and exact I32 route-ID
   tensor, including valid route-row-stride and integer geometry.
3. Weights and outputs have equal concrete layout and every allocation is
   local, non-split, safe, and non-overlapping.
4. Each operation would independently select MMQ rather than short MMVQ or a
   fallback implementation.
5. Both submissions belong to stream zero and the graph has no concurrent
   event plan.

On admission, submit one ID helper and one routed Q8_1 activation conversion,
then submit both original MMQs in graph order. Preserve both F32 outputs and
run GLU normally. On any failed predicate, execute both legacy operations.

The option is private to `ggml-hip`, refuses configuration without HIP, and is
limited to same-type Q2/Q3/Q6/Q8 ROCmFPX weights. It adds diagnostic
per-context counters but no persistent cache or cross-evaluation state.

## Single- and dual-node ownership

The HIP backend context that evaluates the graph owns the mapping, Q8_1
scratch, stream submissions, counters, and fallback. In a single-node run that
is the local backend. In a dual-node RPC run each worker rank independently
owns the same resources for its local graph slice. No rank exposes pointers or
reuse success to another rank, and the RPC protocol is unchanged.

Split buffers are rejected. If rank A admits and rank B rejects, each result is
still produced by its rank's ordinary graph semantics; there is no partial
distributed cache object to commit or roll back. Matched performance testing
must compile and bind the same option state on both workers to avoid ambiguous
attribution.

## Consequences

- Feature-off behavior is unchanged.
- The source can remove one helper and one conversion submission per admitted
  pair, but the end-to-end effect is unknown.
- The short-MMVQ generation lane is unchanged; ordinary one-token generation
  is not an expected beneficiary.
- Valid padded ID row strides remain supported.
- Bias, route mismatch, malformed geometry, split buffers, unsafe views, and
  concurrency fall back without a partial paired submission.
- Existing prompt-QKV and future decode-QKV optimizers must be composition
  tested after rebasing: graph optimization occurs before allocation, and the
  routed-MoE execution selector must remain disjoint and ordered.

## Rejected alternatives

- **A persistent Q8 activation cache:** rejected because pointer identity does
  not prove content, lifetime, stream ownership, or route identity.
- **Cross-rank route/Q8 reuse:** rejected because device pointers and pool
  lifetimes are rank-local and no protocol proves safe ownership.
- **Admit a pair based only on equal shapes:** rejected because different IDs,
  strides, or activation bytes change routing semantics.
- **Fold bias/`ADD_ID` into the first slice:** rejected to keep the execution
  and fallback proof independent of bias routing.
- **Choose new MMQ geometry statically:** rejected without retained gfx1151
  kernel profiles across common shapes.
- **Claim a speed gain from fewer source callsites:** rejected because only a
  matched target A/B can establish performance.

## Promotion gate

Keep this ADR proposed and the option default-off until all of the following
are retained:

- macro-OFF/ON host selectors and source contracts;
- an eight-state executable interleaved QKV/routed-MoE matrix proving
  decode-first, prompt-fallback, MoE adjacency, marker disjointness, and
  topological order for every option combination;
- GPU-less exact-`gfx1151` compile/link in the pinned ROCm toolchain;
- both-on compile and source-order composition with prompt-QKV and decode-QKV
  experiments after rebasing onto their accepted main;
- independent review with no unresolved correctness finding;
- target CPU-reference parity for every admitted format and every routing,
  stride, index-boundary, malformed-group, and fallback control;
- per-rank `1/1/1/2` eligible and `0/2/2/2` legacy-MMQ fallback counters with
  graph capture disabled, plus graph-replay qualification;
- real-model reachability on an ordinary MoE model as well as the large stress
  fixture; and
- matched feature-OFF/ON cache-off prompt/TTFT evidence on both Strix Halo
  machines, with generation reported separately.

Any numerical, routing, feature-off, ownership, graph-order, or fallback
mismatch stops promotion before performance measurement.
