# P06f ROCmFPX Q6 expanded-device view-offset correction

Status: **focused direct-HIP and RPC backend correction qualified; exact-model rank-local execution remains next**

P06e proved that full replicated `Q6_0_ROCMFPX` expert tensors with global
expert IDs execute correctly on local HIP and RPC, but a nonzero expert-axis
view returned a zero peer contribution. P06f localizes and corrects that
backend defect without changing generic RPC serialization or MiniMax routing.

ROCmFPX retains packed GGUF byte strides in `ggml_tensor::nb[]` and
`view_offs`, while its Strix Halo HIP buffer expands each Q6 block on device.
Generic view initialization initially points `tensor->data` at
`view_src->data + view_offs`, which applies a packed offset directly to the
expanded allocation. The HIP buffer initializer now converts a block-aligned
packed Q6 view offset with the existing `ggml_cuda_tensor_offset()` helper.
It rejects a mismatched view type or unaligned packed offset. All other tensor
types, non-view tensors, split buffers, RPC wire fields, and compute kernels
retain their prior behavior.

## Focused qualification

Both nimo-1 and nimo-2 built byte-identical Release `gfx1151` test and RPC
binaries with AMD ROCm Clang 22, HIP, RPC, forced MMQ, no VMM, and Vulkan off.
The purpose-built oracle creates four Q6 expert matrices, then compares:

- full storage with global expert IDs 2 and 3; and
- a two-expert view beginning at packed byte offset 53,248 with local IDs 0
  and 1.

On nimo-2 `ROCm0`, the two results matched with NMSE 0 and maximum absolute
error 0. Nimo-2 then ran the same executable through `RPC0` against a
disposable nimo-1 ROCm peer; that result also matched with NMSE 0 and maximum
absolute error 0. The inherited ROCmFPX Q6 `MUL_MAT_ID` filter passed 73/73
cases. Its compiler emitted only six pre-existing deprecated-enum warnings in
`test-backend-ops.cpp`; the candidate and focused test built without warnings.

The first RPC invocation used `10.44.0.2`, nimo-2's own rail address, instead
of the nimo-1 peer at `10.44.0.1`. It failed to connect and aborted before
backend creation, allocation, or graph execution. The failed non-execution is
retained separately and is not counted as a candidate failure or a passing
trial.

This is a correctness milestone, not a performance result. The correction is
outside the generation hot path and adds work only when initializing an
expanded-device Q6 view. P07 remains the matched feature-off performance
control; strict final non-inferiority remains open.

## Rollback, provenance, and next seam

The known-good nimo-1 coordinator was stopped before the nimo-2 worker. After
the RPC canary, the disposable peer was removed and the nimo-2 worker was
restored before nimo-1. The server completed its normal 3m27s model load and
returned HTTP 200; both services are active with zero restarts and their
original binary hashes.

Raw evidence is manifest-verified and bundled separately on both nodes. All
five immutable reference clones retain their exact locked commits and trees
with clean worktrees. The implementation is target-native and imports no donor
expression, GPL llama-ai code, CachyLLama code, dependency, persistent write,
WebUI, model mutation, deployment replacement, notice, or SBOM change.

P06f does not itself enable physical expert sharding. The next milestone may
replace P06e's replicated full-tensor/global-ID shadow branches with explicit
96-expert views and rank-local IDs under the existing strict default-off
canary, then run one exact-artifact correctness request before any performance
claim.
