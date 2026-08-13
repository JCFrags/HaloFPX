# ADR-0055: strict n=1 ROCmFPX Q/K/V MMVQ Q8_1 activation reuse

Status: proposed; host contracts implemented. Target execution and promotion
remain closed under [issue #41](https://github.com/JCFrags/HaloFPX/issues/41).

Date: 2026-08-12

## Context

For one-token generation, the local HIP path dispatches separate quantized Q,
K, and V projections through MMVQ. Each ordinary `ggml_cuda_mul_mat_vec_q`
call converts the exact shared F32 activation to Q8_1 before invoking its
unchanged type-selected MMVQ consumer. The three projections can therefore
submit the same conversion three times.

[Issue #42](https://github.com/JCFrags/HaloFPX/issues/42) owns Q/K/V activation
reuse across prompt and generation paths. This decision is only its smallest
model-general generation slice. MiniMax is not a special admission condition.
GQA must remain supported because Q output width can differ from K/V output
width.

The target platform is CachyOS Linux with ROCm on exact `gfx1151` AMD Strix
Halo hardware. Windows and WSL are development environments, not the target.
The RPC backend has no graph optimizer, so this first decision does not admit
remote RPC splits or establish dual-node reachability.

## Decision

Add literal-default-`OFF`
`GGML_HIP_ROCMFPX_MMVQ_QKV_Q8_REUSE`, defined privately for `ggml-hip` and
rejected by CMake when requested without HIP.

The production backend optimizer runs before graph copy and allocation. On
exact `gfx1151`, it may recognize and stably compact one semantic Q/K/V group
to adjacent Q, K, V nodes. It publishes an internal role marker only after the
complete candidate order passes topology, grouping, alias-write, and lifetime
validation. A reused graph first loses any marker previously owned by this
feature and must requalify. An admitted group owns the single-stream order,
clears the CUDA/HIP concurrent-event plan, and returns before the optional QKV
concurrency optimizer.

Runtime dispatch repeats the material predicates and requires all of the
following:

- exactly one activation row (`n=1`), with higher activation/output dimensions
  also collapsing to one through independent geometry checks;
- exactly three ordinary, compute-marked `MUL_MAT` nodes in Q, K, V order;
- canonical `Qcur-N`, `Kcur-N`, `Vcur-N` outputs and corresponding
  `blk.N.attn_q/k/v.weight` tensors from the same layer;
- one exact F32 activation tensor and data pointer;
- distinct Q/K/V weights with the same concrete type from Q2/Q3/Q6/Q8
  ROCmFPX only;
- F32 outputs, default precision/hint, independent valid Q/K/V geometry, and
  MMVQ eligibility for every consumer;
- owning contiguous activation, weights, and outputs; non-null exact local
  HIP allocations; no overlapping ranges and no split/RPC buffers;
- stream zero with no concurrent-event plan; and
- exact optimizer markers on three adjacent roles.

These constraints refuse fused WQKV, LoRA or an extra shared-activation
matmul, bias/clamp/scale consumers, `MUL_MAT_ID`, views, mixed types, wrong
role order, and an intervening in-place write rooted at a protected activation,
weight, or output.

An admitted call clears compute-weight padding in graph order, allocates one
execution-local Q8_1 pool object, submits one existing
`quantize_row_q8_1_cuda`, and invokes the unchanged
`mul_mat_vec_q_switch_type` dispatcher for Q, K, then V with each consumer's
own dimensions and strides. The allocation remains alive through all four
same-stream submissions. The dispatcher returns two skipped graph nodes after
preserving the three ordinary F32 outputs.

Any failed runtime predicate uses the ordinary independent nodes. Planning is
still conservative because a late fallback has already declined optional QKV
concurrency; this is a target regression gate, not assumed harmless.

## Feature-off and evidence boundary

When the option is off, the planner, markers, runtime branch, helper,
diagnostic counters, and feature advertisement are excluded at compile time.
The ordinary MMVQ source remains the execution path. Host OFF/ON graph
contracts require OFF dispatch to preserve node order and `op_params`
exactly.

Feature-on per-context counters report optimizer groups, triple dispatches,
Q8_1 conversion submissions, and MMVQ submissions. They count host
submissions, not captured-graph replays and not time saved. The target
discriminator is `1/1/1/3` for planned/triple/conversion/MMVQ versus an
eligible ordinary control's three conversions and three MMVQs. A malformed
feature-on graph must remain `0/0/3/3`.

[MEASURED] In a local WSL host-only build on 2026-08-12, the OFF selector, ON
selector, and source contract passed 3/3. `test-backend-ops` compiled and its
feature case correctly skipped on CPU. This is development evidence only.

[OPEN] No `gfx1151` compile/link, HIP numerical run, real-model reachability,
single-node or dual-node execution, output/KV parity, or performance result is
claimed. A pinned GPU-less ROCm workflow is included as a compile/link gate,
but it cannot run before publication and does not substitute for target
correctness.

## Promotion gates

After issue #41 explicitly admits a maintenance window, promotion requires:

1. exact OFF/ON CachyOS/ROCm build identities and feature-off binary/source
   provenance;
2. CPU-reference parity for all four types at n=1 GQA and MHA shapes;
3. exact `1/1/1/3` eligible counts and `0/0/3/3` malformed fallback counts
   with graph capture disabled;
4. changed-activation replay, graph capture, `GGML_CUDA_GRAPH_OPT=0|1`, alias,
   and late-fallback checks;
5. real-model reachability with exact model and source hashes;
6. matched cache-off generation A/B on each single node before any dual-node
   interpretation; and
7. a separate RPC/protocol decision before remote-rank or dual-node coverage
   is claimed.

Any numerical, feature-off, graph-order, lifetime, fallback, or concurrency
mismatch stops performance work.

## Consequences and rollback

The feature can remove two conversion submissions per admitted attention
layer/token, but MMVQ weight work remains unchanged, so the likely gain is
bounded and no speed claim follows from source structure. The strict names
and generic separate-Q/K/V form intentionally trade reachability for a
reviewable first slice.

Rollback removes the option, private macro, optimizer/dispatch/helper code,
metrics proc, tests/workflow, build-script passthrough, and this record. There
is no stored format or protocol migration.
