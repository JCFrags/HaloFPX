# ROCmFPX dense FFN Q8_1 activation reuse

Status: default-off implementation candidate for
[GitHub issue #29](https://github.com/JCFrags/HaloFPX/issues/29). The exact
`gfx1151` compile/link gate passed at
[`3402aa7`](evidence/rocmfpx-ffn-q8-reuse-3402aa7/README.md); target runtime
correctness, launch-count, and performance qualification remain open.

Current source base: `b77f2bce6e7875ab065e09894f45915585c9f156`.
The target compile used the pre-rebase equivalent commit `3402aa7`; the
[compile receipt](evidence/rocmfpx-ffn-q8-reuse-3402aa7/README.md) maps that
source explicitly to rebased commit `8369bfa2`.

## Purpose and boundary

[VERIFIED] The ordinary HIP MMQ path converts an F32 activation to its Q8_1
MMQ layout inside each `ggml_cuda_mul_mat_q` call. Two adjacent dense FFN gate
and up projections with the exact same activation therefore enter that
conversion path independently when both are dispatched as MMQ. This statement
describes the pinned source path; it is not a target performance result.

The candidate adds `GGML_HIP_ROCMFPX_FFN_Q8_REUSE`, default `OFF`. CMake
defines it privately for `ggml-hip`; requesting it without `GGML_HIP=ON` is a
configuration error. CUDA, Vulkan, CPU, RPC, and feature-off execution retain
their previous dispatch.

The first implementation slice is intentionally narrower than issue #29's
full proposed six-format selector. It admits only:

- `GGML_TYPE_Q2_0_ROCMFPX`;
- `GGML_TYPE_Q3_0_ROCMFPX`;
- `GGML_TYPE_Q6_0_ROCMFPX`;
- `GGML_TYPE_Q8_0_ROCMFPX`.

ROCmFP4, ROCmFP4 FAST, stock quantizations, and non-quantized weights remain
controls. They are not silently admitted by layout similarity.

## Fail-closed selector

Reuse is attempted only inside the existing conservative no-bias
`{ MUL_MAT, MUL_MAT, GLU }` graph match. Every candidate must also have:

- ordinary dense `MUL_MAT` nodes, never `MUL_MAT_ID` or routed MoE;
- the same F32 activation tensor and data pointer;
- more than eight activation columns, so the ordinary MMVQ lane remains
  unchanged;
- F32 destinations with matching shape and stride;
- equal whitelisted weight types, shapes, and strides;
- two independently MMQ-eligible operations;
- local, non-split HIP buffers for both weights, the activation, and outputs;
- a contiguous, non-view activation and no unsafe compute-buffer weight view;
- no RPC, split-buffer, bias, or persistent pointer-based cache.

Any failed predicate runs the original nodes independently. The selector's
pure contract is compiled and tested in both macro modes on a host without
ROCm so default-off and negative cases do not depend on target availability.
The backend-operation suite also contains prompt-sized dense FFN graphs for
each admitted format at 9 and 32 columns, plus an 8-column MMVQ-boundary
control and a stock-Q4 type control. They become paired-path correctness tests
only when run against an enabled HIP backend.

## Execution-local lifetime

For an admitted pair, the lifetime is exactly:

```text
allocate Q8_1 pool scratch
  -> quantize the shared F32 activation once
  -> launch graph-order MMQ A
  -> launch graph-order MMQ B
  -> release scratch
  -> run the existing GLU node normally
```

The Q8_1 pointer is not stored in a tensor, backend context, graph, stream,
device, process, or rank cache. Both MMQs use the current backend stream. The
pair path preserves both ordinary F32 projection outputs and skips only the
second already-executed `MUL_MAT` node.

## Qualification state

[OPEN] A host selector or successful HIP compile cannot establish numerical
correctness or speed on `gfx1151`. Before enabling this option outside an
isolated comparison build, retain all of the following from the two CachyOS
Strix Halo targets:

1. exact source commit, compiler/ROCm tuple, CMake cache, binary hashes, model
   hash, and service/process isolation receipt;
2. feature-off and feature-on CPU-reference output checks for all four admitted
   formats at prompt and tail shapes, including graph replay;
3. negative runtime selectors for eight columns, distinct activation tensors,
   a bias graph, stock Q4, a split buffer, an unsafe view, and `MUL_MAT_ID`;
4. a matched kernel trace showing two Q8_1 conversions and two MMQs with the
   option off, then one conversion and two MMQs with it on;
5. matched cache-off prompt-processing and time-to-first-token samples on each
   target, with generation reported separately.

No prompt-processing gain is claimed by this source slice. Promotion requires
retained target evidence and should keep the option default-off until the
correctness and matched-performance gates pass.

## Focused development commands

```bash
cmake -S . -B build/ffn-q8-reuse-host \
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
cmake --build build/ffn-q8-reuse-host --parallel 2 --target \
  test-halofpx-rocmfpx-ffn-q8-reuse-off \
  test-halofpx-rocmfpx-ffn-q8-reuse-on
ctest --test-dir build/ffn-q8-reuse-host --output-on-failure \
  -R '^test-halofpx-rocmfpx-ffn-q8-reuse-(off|on|source-contract)$'
```

For a target HIP build, configure two fresh build directories that differ only
in `-DGGML_HIP_ROCMFPX_FFN_Q8_REUSE=OFF|ON`. Do not infer feature admission
from a macro present in `CMakeCache.txt`; retain the actual HIP compile command
and the loaded shared-library identity.
