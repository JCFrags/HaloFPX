# ADR-0053: bounded ROCmFPX Q/K/V Q8_1 activation reuse

**Status:** proposed

## Context

Separate ROCmFPX Q, K, and V prompt MMQs can convert one exact F32 activation
to the same Q8_1 layout three times. Real graphs do not normally place those
raw matrix multiplies next to one another, and moving them after allocation
can invalidate allocator lifetime assumptions. Existing optional graph
concurrency can also map Q/K/V branches to different streams.

The dense-FFN activation-reuse work merged in PR #45 supplies the local pool
scratch and type-selected MMQ pattern, but it is not Q/K/V target evidence.
Issue #42 owns this separate candidate. Issue #41 currently blocks target
execution.

## Decision

Add a default-off HIP option that admits only a bounded single-stream
`gfx1151` path:

1. Before graph copy and allocation, recognize exact ordinary Q/K/V projection
   semantics and stably compact the three raw matrix multiplies to Q, K, V.
2. Reject compaction across an in-place write rooted at the shared activation
   or any Q/K/V weight, then mark only the proven nodes with internal
   graph-order provenance.
3. At execution, recheck buffer locality, allocation/view safety, exact shared
   activation, independent geometry, MMQ eligibility, compute flags, target
   architecture, and single-stream state.
4. Allocate one execution-local Q8_1 scratch buffer, convert once, submit the
   Q, K, and V MMQs in graph order, then release the scratch.
5. Preserve the three ordinary F32 outputs and all downstream operations.
6. On any failed graph or runtime predicate, execute the original independent
   matrix multiplies.

Admission is limited to same-concrete-type Q2/Q3/Q6/Q8 ROCmFPX weights and
more than eight activation columns. Different Q and K/V output widths are
supported. Persistent pointer caches, mixed types, fused WQKV, LoRA or extra
activation matmuls, bias/clamp in the standard builder path, `MUL_MAT_ID`,
views, split/RPC buffers, and concurrent-event graphs are refused.

The eligible path suppresses the later optional graph-concurrency transform.
This is a correctness boundary, not a performance conclusion.

## Consequences

- Feature-off source and execution remain unchanged.
- The candidate targets prompt processing, not ordinary token generation.
- Two activation-conversion submissions can be removed only when runtime
  admission succeeds.
- GQA-like unequal Q versus K/V widths do not force a fallback.
- Exact names keep the first slice narrow and leave broader model/architecture
  reachability open.
- Alias-write rejection keeps graph-topological validity from hiding a changed
  memory ordering dependency.
- A late runtime fallback can still lose optional graph concurrency because
  semantic planning happened before allocation. This must be measured and is
  a target stop gate.
- Per-context metrics are feature-on diagnostics and count host submissions;
  captured graph replay needs separate qualification.

## Rejected alternatives

- **Execute nonadjacent K/V early after allocation:** rejected because graph
  allocation and alias lifetimes were planned for the original order.
- **Rely only on graph sources while reordering:** rejected because an
  intervening in-place view can write the same allocation without becoming a
  source of the later projection.
- **Hold Q8 scratch across arbitrary branch operations:** rejected because
  lifetime and concurrent-stream ownership become ambiguous.
- **Use persistent activation-pointer caching:** rejected because addresses do
  not prove content identity or safe lifetime.
- **Admit mixed ROCmFPX types because current layouts happen to match:**
  rejected for this slice; exact type equality is the conservative invariant.
- **Reuse fused WQKV:** rejected because that path already owns one projection
  operation and does not need this triple.
- **Claim PR #45's target compile for Q/K/V:** rejected because Q/K/V changes
  modify `ggml-cuda.cu` and `mmq.cu` after that receipt.

## Promotion gate

The decision remains proposed until an isolated target window supplies:

- exact `gfx1151` OFF/ON compile and binary identity;
- CPU-reference parity for every admitted type and negative fallback;
- `1` triple, `1` conversion, `3` MMQs for an eligible whole-graph run and
  `0/3/3` for the distinct-activation fallback with graph capture disabled;
- real-model graph reachability plus graph-replay and concurrency regression
  checks; and
- matched per-node cache-off prompt/TTFT evidence with FFN reuse fixed
  identically, while generation is reported separately.

Any numerical, feature-off, fallback, graph-lifetime, or concurrency mismatch
stops promotion before performance measurement.

The feature-on HIP translation units also have a pinned, GPU-less `gfx1151`
compile/link CI job. The hosted runner was blocked before execution by account
billing, so the exact pinned job was reproduced locally with rootless Podman.
Fresh OFF and ON `ggml-hip` trees compiled and linked 170/170 with the expected
macro and architecture flags. This closes the source-build prerequisite, not
the blocked target runtime gate.
