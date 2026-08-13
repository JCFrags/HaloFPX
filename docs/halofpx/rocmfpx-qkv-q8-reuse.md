# ROCmFPX Q/K/V Q8_1 activation reuse

Status: default-off source candidate for
[GitHub issue #42](https://github.com/JCFrags/HaloFPX/issues/42).

Publication-source head: `760276e39123622aadf5ef915e9c2b4f92172f8f`.
Core implementation/hardening head:
`7a6984c5a03df384be269d1c266ee993fa2184ea`. Exact base:
`3758febacfc07fdc6e84b63637131b02d413de59`. The first two commits contain the
source candidate and independent-review hardening; the third corrects the
pinned container's absolute `hipconfig` command and binds it in the source
contract. The base includes merged PR #45's dense-FFN Q8_1 reuse, PR #44's
target OOM authority, and PR #49's longest-prefix selector; none is Q/K/V
target qualification.

## Purpose and claim boundary

**[VERIFIED]** In the pinned source, three ordinary HIP MMQ dispatches for
separate Q, K, and V projections each enter `ggml_cuda_mul_mat_q` and convert
their F32 activation to Q8_1 independently. **[INFERENCE]** Reusing one exact
conversion can remove two conversion submissions when all three projections
share the same activation and Q8 layout. This is a source opportunity, not a
measured speed result.

The candidate adds `GGML_HIP_ROCMFPX_QKV_Q8_REUSE`, default `OFF`. CMake
defines it privately for `ggml-hip`; enabling it without `GGML_HIP=ON` is a
configuration error. It changes neither persistent prompt/KV cache state nor
the on-disk cache product. The reused Q8_1 data is execution-local pool
scratch.

This slice is primarily a prompt-processing candidate. It requires more than
eight activation columns and therefore excludes ordinary one-token generation.
No decode or generation-throughput improvement is claimed.

## Conservative admission

The source admits only Q2, Q3, Q6, and Q8 ROCmFPX weights on exact `gfx1151`
HIP. Q, K, and V must use the same concrete type. The graph recognizer requires:

- three distinct ordinary `MUL_MAT` nodes sharing one exact F32 activation;
- exact projection names `Qcur-N`, `Kcur-N`, and `Vcur-N` for one layer;
- exact weights `blk.N.attn_q.weight`, `attn_k.weight`, and `attn_v.weight`;
- exactly one direct `RESHAPE` consumer for each raw projection;
- default matrix-multiply precision and hint;
- no fourth matrix multiply from the same activation;
- independently valid Q, K, and V geometry, including different Q versus K/V
  output widths for grouped-query attention.

The runtime selector rechecks exact shared tensor and data identity, F32
activation and outputs, local non-split buffers, owning contiguous tensors,
pairwise non-overlapping allocation ranges, individual MMQ eligibility, all
three compute flags, exact `gfx1151`, stream zero, and the absence of any
concurrent-event plan.

These gates deliberately reject mixed ROCmFPX types, ROCmFP4, stock quants,
fused WQKV, LoRA or other extra activation matmuls, bias/clamp in the standard
builder path, `MUL_MAT_ID`, split/RPC buffers, unsafe views, and concurrent
streams. Any failed predicate leaves the three original operations to execute
independently.

Before moving a projection, the planner also scans the full compacted interval
for non-metadata view-producing operations whose allocation root is the shared
activation or any Q/K/V weight. Such an operation is an in-place write that
could otherwise make an earlier projection observe different bytes. The group
is rejected before it is marked or reordered. Host negatives cover a crossed
activation scale and a copy into the K weight; metadata-only activation views
and branch-local in-place writes remain positive controls.

**[OPEN]** Exact tensor names make this candidate conservative rather than
architecture-general today. The standard builder paths were source-audited,
but real-model graph reachability has not been demonstrated across supported
architectures. A custom graph that moves bias after the admitted reshape is
outside the recognized standard-builder proof and must remain an explicit
negative control before broadening admission.

## Pre-allocation ordering and execution

Real prompt Q/K/V matrix multiplies are separated by reshape, normalization,
RoPE, or cache branch nodes. The scheduler calls the backend graph optimizer
before graph copy and allocation. Under the feature, that hook recognizes an
eligible group and stably compacts only its raw projection nodes to canonical
Q, K, V order at the earliest projection position. It preserves every other
node's relative order and requires all in-graph projection sources to precede
the insertion point.

For an admitted group, the execution lifetime is:

```text
pre-allocation stable Q/K/V reorder
  -> allocate one Q8_1 pool scratch buffer
  -> convert the shared F32 activation once
  -> submit Q MMQ
  -> submit K MMQ
  -> submit V MMQ
  -> release scratch after all three same-stream submissions
```

Only K and V raw `MUL_MAT` nodes are skipped after the triple helper has
produced their ordinary F32 outputs. Downstream reshape, RoPE, cache, and
attention nodes run normally. The candidate does not change MMQ arithmetic.

An eligible group owns the graph-optimization hook: it clears stream context
and returns before the later optional concurrency optimizer. This avoids a
K/V consumer race, but it can sacrifice unrelated concurrency if a later
runtime-only predicate falls back. Matched target testing must treat any
feature-on prompt or graph-replay regression as a stop condition.

## Runtime evidence seam

A versioned, feature-on-only private backend procedure,
`halofpx_rocmfpx_qkv_q8_reuse_metrics_v1`, synchronizes and resets or snapshots
per-backend-context host-submission counters:

- graph groups planned;
- triple dispatches;
- Q8_1 conversions submitted; and
- MMQs submitted.

The backend-operation case builds GQA-like unequal widths, invokes the
registered production backend graph optimizer before allocation, runs the
whole graph once, and compares Q, K, V, and terminal outputs against CPU. With
GPU graphs disabled, the target expectations are:

| Enabled-build case | Planned by backend optimizer | Triple | Conversions | MMQs |
|---|---:|---:|---:|---:|
| exact shared activation | 1 | 1 | 1 | 3 |
| distinct V activation fallback | 0 | 0 | 3 | 3 |

Separate host graph tests prove one eligible/moved group, the alias barriers,
and idempotent second planning. Counters measure host submissions, not captured
graph replays; use `GGML_CUDA_DISABLE_GRAPHS=1` for the exact-count test.

## Qualification state

**[VERIFIED]** Host-only selector/reorder and exact source-contract tests pass
in macro-OFF and macro-ON builds, and `test-backend-ops` compiles with the Q/K/V
case. A representative existing CPU `ADD` lane passes 54/54 after the harness
pre-allocation hook change. These checks prove source shape and host graph
behavior only. The retained receipt contains the raw commands, outputs,
toolchain, cache, compile database, hashes, and review boundary.

An off-target GitHub Actions job pins AMD's ROCm 7.2.4 development image by
digest, compiles and links `ggml-hip` serially for exact `gfx1151`, and verifies
that `ggml-cuda.cu` and `mmq.cu` contain both the feature macro and architecture
flag. Its result is not part of the local host receipt and remains unverified
until the published workflow completes.

**[OPEN]** Issue [#41](https://github.com/JCFrags/HaloFPX/issues/41) blocks
target access after the production HMM/OOM incident. This Q/K/V change has no
target runtime parity, model reachability, launch trace, graph replay, or
matched performance evidence. An off-target compile cannot qualify those
runtime claims. PR #45's earlier target compile does not qualify the
Q/K/V-modified translation units.

Promotion requires an isolated maintenance window and retained evidence for:

1. exact OFF/ON `gfx1151` HIP builds from this Q/K/V commit;
2. CPU-reference parity for all four admitted types at 9 and 32 columns,
   including unequal output widths and graph replay;
3. exact `1/1/3` eligible counters and `0/3/3` semantic fallback counters with
   GPU graphs disabled;
4. real-model layer reachability without LoRA, bias, fused WQKV, split buffers,
   or unsafe views;
5. matched cache-off prompt/TTFT measurements on each Strix Halo node, holding
   the FFN Q8-reuse option identically `OFF` in both conditions; and
6. no feature-off, generation, concurrency, or graph-replay regression.

Do not report a prompt speed gain unless those matched retained samples pass.

## Focused host commands

```bash
cmake -S . -B build/qkv-q8-reuse-host \
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
cmake --build build/qkv-q8-reuse-host --parallel 2 --target \
  test-halofpx-rocmfpx-qkv-q8-reuse-off \
  test-halofpx-rocmfpx-qkv-q8-reuse-on \
  test-backend-ops
ctest --test-dir build/qkv-q8-reuse-host --output-on-failure \
  -R '^test-halofpx-rocmfpx-qkv-q8-reuse-(off|on|source-contract)$'
```
