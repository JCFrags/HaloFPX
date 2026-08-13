# ROCmFPX strict n=1 MMVQ Q/K/V Q8_1 activation reuse

Status: default-off source candidate for
[issue #42](https://github.com/JCFrags/HaloFPX/issues/42). The governing
decision is [ADR-0055](decisions/0055-rocmfpx-strict-n1-mmvq-qkv-q8-reuse.md).

## What this slice changes

This is a local HIP generation-path optimization for exact `gfx1151`. It
recognizes ordinary separate Q/K/V projections sharing one F32 n=1 activation,
orders the three raw nodes before allocation, converts that activation to
Q8_1 once, and sends the same scratch to the existing MMVQ dispatcher three
times. Q, K, and V keep independent output widths, so GQA is admitted.

The option is:

```text
GGML_HIP_ROCMFPX_MMVQ_QKV_Q8_REUSE=OFF
```

It is threaded through the Strix and primary matched build scripts and remains
off unless explicitly selected. It does not change the prompt/MMQ FFN option.

## Fail-closed boundary

The candidate accepts only same-concrete-type Q2/Q3/Q6/Q8 ROCmFPX weights,
strict n=1, canonical same-layer Q/K/V names, exact activation object/data
identity, default ordinary matmul parameters, owning contiguous nonoverlapping
local HIP allocations, three independent valid geometries, MMVQ eligibility,
and an optimizer-published Q/K/V role capability.

It refuses fused WQKV, LoRA/extra matmuls, bias/clamp/scale, `MUL_MAT_ID`,
views, mixed types, split/RPC buffers, role reorder, protected in-place writes,
and concurrent stream plans. Any miss retains three ordinary MMVQ calls.

The graph optimizer is the registered production seam invoked by the scheduler
before graph copy/allocation. The HIP backend integration case calls that
registered optimizer rather than invoking the host planner directly.

## Current qualification

[MEASURED] WSL host development on 2026-08-12:

```text
test-halofpx-rocmfpx-mmvq-qkv-q8-reuse-off                PASS
test-halofpx-rocmfpx-mmvq-qkv-q8-reuse-on                 PASS
test-halofpx-rocmfpx-mmvq-qkv-q8-reuse-source-contract    PASS
test-backend-ops feature case on CPU                       SKIP (expected)
```

The host suite covers the four exact types, strict n=1 boundary, GQA and MHA,
every selector boolean, mixed/non-ROCmFPX types, malformed names/roles/layers,
distinct activation, extra matmul, bias, views, `MUL_MAT_ID`, nondefault
parameters, protected crossed writes, stable topology/order, stale-marker
clearing, idempotence, and feature-off graph/parameter identity. The source
contract proves one pool allocation, one conversion call, ordered Q/K/V calls
through the unchanged MMVQ dispatcher, production optimizer ownership, and
versioned counters.

[VERIFIED] Exact source `b59f62e1e34d6a3176d25583ff6d8f311b91e242`
passed clean GPU-less OFF/OFF and ON/ON `gfx1151` HIP compile/link, 170/170 in
both modes. The retained [composition receipt](evidence/rocmfpx-qkv-composition-b59f62e1-gfx1151-hip-compile/README.md)
proves both prompt and decode feature macros absent/present as expected across
`ggml-cuda.cu`, `mmq.cu`, and `mmvq.cu`. This is compile/link evidence only.

[OPEN] No target was accessed because issue #41 remains the authority gate.
No HIP runtime correctness, reachability, counter, replay, parity, or speed
result exists.

Remote RPC graph splits do not run the local HIP graph optimizer today. This
candidate therefore has no dual-node reachability claim; a later
protocol-aware optimizer/capability design is required.

## Focused host commands

```bash
cmake -S . -B build/mmvq-qkv-host -G Ninja \
  -DCMAKE_BUILD_TYPE=Debug \
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
cmake --build build/mmvq-qkv-host --parallel 4 --target \
  test-halofpx-rocmfpx-mmvq-qkv-q8-reuse-off \
  test-halofpx-rocmfpx-mmvq-qkv-q8-reuse-on \
  test-backend-ops
ctest --test-dir build/mmvq-qkv-host --output-on-failure \
  -R '^test-halofpx-rocmfpx-mmvq-qkv-q8-reuse-(off|on|source-contract)$'
build/mmvq-qkv-host/bin/test-backend-ops test \
  -o HALOFPX_ROCMFPX_MMVQ_QKV_Q8_REUSE
```

The final command must skip on CPU. On an admitted feature-on `gfx1151` HIP
backend it is the production-optimizer/correctness/count discriminator.
