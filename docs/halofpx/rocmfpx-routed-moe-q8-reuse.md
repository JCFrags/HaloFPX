# ROCmFPX routed-MoE gate/up Q8_1 preparation reuse

Status: default-off source candidate under
[ADR-0060](decisions/0060-rocmfpx-routed-moe-q8-reuse.md). Target runtime and
performance qualification are blocked by issue #41.

Initial audited base: `9bfccf25d43af0c446df591035e9cdac0b74d6c0`.
Publication head: `[OPEN]` until the candidate is rebased on the next accepted
`main`, reviewed, and published as a draft pull request.

## Purpose and claim boundary

**[VERIFIED]** The existing routed MMQ path independently prepares the same
route mapping and routed Q8_1 activation in each gate/up `MUL_MAT_ID` call.
The candidate adds `GGML_HIP_ROCMFPX_MOE_Q8_REUSE`, default `OFF`, to prepare
those exact inputs once for an already-adjacent no-bias gate/up pair.

The pair still submits two independent MMQs, retains both ordinary F32
projection outputs, and executes the existing GLU node normally. It changes no
matrix arithmetic, persistent cache state, model format, RPC wire, or expert
ownership rule. No prompt or generation speed improvement is claimed.

## Fail-closed admission

The first slice admits only Q2/Q3/Q6/Q8 ROCmFPX weights on exact `gfx1151`
HIP. It requires:

- an existing adjacent no-bias `{ MUL_MAT_ID, MUL_MAT_ID, GLU }` graph group;
- exact shared F32 activation tensor/data and exact shared I32 route-ID
  tensor/data;
- equal weight type, shape, and stride and equal output shape and stride;
- default matrix-multiply precision and hint;
- valid token, expert-count, experts-used, route-row-stride, output, and
  integer-conversion geometry;
- valid individual MMQ selection after the architecture-specific short-MMVQ
  gate;
- local, owning, contiguous, non-split weight/activation/output allocations;
- pairwise non-overlapping weights, activation, IDs, outputs, and GLU output;
  and
- stream zero with no concurrent-event plan.

The ID tensor may be a padded-row-stride view, matching the standard
`n_expert_used < n_experts` graph representation. The selector validates its
element and row-stride geometry but does not synchronously copy device IDs to
the host. Device ID value validity remains the existing `MUL_MAT_ID`
precondition; reuse uses the exact same ID bytes for both operations.

Bias/`ADD_ID`, distinct route tensors, distinct activations, mixed types,
short-MMVQ cases, malformed geometry, unsafe views, split/RPC buffers,
non-gfx1151 devices, and concurrent plans all retain legacy execution.

## Execution and rank ownership

For an admitted local pair:

```text
allocate one local route-map scratch set
  -> submit one ID helper
  -> allocate one local Q8_1 scratch buffer
  -> submit one routed activation gather/conversion
  -> submit gate MMQ
  -> submit up MMQ
  -> release local scratch after same-stream submissions
  -> execute ordinary GLU
```

Single-node execution owns all state in its one HIP backend context. In a
dual-node run, every RPC worker rank independently owns and frees its own
scratch and independently falls back. Nothing is shared across ranks; there
is no coordinator-side semantic cache. A worker built with the option off is
behaviorally unchanged. Matched A/B evidence must nevertheless bind both
worker builds so attribution has one cause.

## Correctness and evidence seams

Macro-OFF and macro-ON host tests exercise every selector predicate
independently, including route identity, route stride/type metadata, expert
and integer bounds, and malformed group/bias state. The backend-operation case
adds:

- eligible 9- and 32-token pairs for all four admitted formats;
- deterministic route rows containing expert `0` and expert `n_experts - 1`;
- a valid padded-row-stride ID view;
- a one-token MMVQ fallback;
- a distinct-ID fallback; and
- a bias/`ADD_ID` fallback.

A feature-on-only versioned backend procedure reports local host submissions.
With GPU graph capture disabled, one eligible pair must report `1` pair,
`1` ID helper, `1` Q8_1 conversion, and `2` MMQs. The distinct-ID and bias
fallbacks must report `0/2/2/2`; the short-MMVQ control reports `0/0/0/0` for
these MMQ-specific counters. Every retained output is compared with CPU.

The source contract also checks that the pair helper contains exactly one ID
helper call, one Q8_1 conversion call, and one shared MMQ callsite invoked in
A-then-B order.

An eight-state executable host composition matrix interleaves one complete
routed-MoE gate/up/GLU group between Q and K/V. It exercises the production
planner order for both paths: strict n=1 decode owns its eligible graph when
enabled, while a 32-row graph falls through to prompt QKV when enabled. The
matching Q/K/V group becomes adjacent only in the owning feature states,
without changing the MoE group's adjacency, topological order, or marker
bytes. The matrix compiles and executes every
prompt-QKV/decode-QKV/routed-MoE ON/OFF combination. The source contract also
scans every MoE-owned selector, dispatch, paired-MMQ, and metrics region for
accidental QKV marker ownership.

## Qualification state

**[VERIFIED]** At the current source-candidate boundary, macro-OFF and macro-ON
host selectors, the executable QKV/MoE composition graph, and the exact source
contract pass, and `test-backend-ops` compiles with the new graph cases. These
facts establish host/source shape only.

**[OPEN]** A pinned GPU-less `gfx1151` HIP compile, feature-composition checks,
independent review, target numerical parity, graph replay, real-model
reachability, and matched performance remain required. Issue #41 blocks all
target work. Do not turn the feature on in ordinary builds or report a gain
from compile or submission-count evidence.

## Focused host commands

```bash
cmake -S . -B build/moe-q8-reuse-host -G Ninja \
  -DCMAKE_BUILD_TYPE=Release \
  -DBUILD_SHARED_LIBS=OFF \
  -DGGML_NATIVE=OFF \
  -DGGML_HIP=OFF \
  -DGGML_RPC=OFF \
  -DLLAMA_BUILD_TESTS=ON \
  -DLLAMA_BUILD_TOOLS=OFF \
  -DLLAMA_BUILD_EXAMPLES=OFF \
  -DLLAMA_BUILD_SERVER=OFF \
  -DLLAMA_BUILD_WEBUI=OFF \
  -DLLAMA_OPENSSL=OFF
cmake --build build/moe-q8-reuse-host --parallel 2 --target \
  test-halofpx-rocmfpx-moe-q8-reuse-off \
  test-halofpx-rocmfpx-moe-q8-reuse-on \
  test-halofpx-rocmfpx-qkv-moe-composition-matrix \
  test-backend-ops
ctest --test-dir build/moe-q8-reuse-host --output-on-failure \
  -R '^test-halofpx-rocmfpx-(moe-q8-reuse-(off|on|source-contract)|qkv-moe-composition-p[01]-d[01]-m[01])$'
```
