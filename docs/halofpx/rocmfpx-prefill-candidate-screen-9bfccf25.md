# Model-general ROCmFPX prefill candidate screen at `9bfccf25`

Status: source audit and candidate ranking. This is not a benchmark or a speed
claim. The exact audited base is
`9bfccf25d43af0c446df591035e9cdac0b74d6c0`.

## Scope and source observations

**[VERIFIED]** The pinned HIP source selects `ggml_cuda_mul_mat_q` for routed
`MUL_MAT_ID` operations after the short-batch MMVQ gate and an independent
`ggml_cuda_should_use_mmq` decision. In that MMQ path, every call separately:

1. allocates `ids_src1`, `ids_dst`, and `expert_bounds`;
2. submits `ggml_cuda_launch_mm_ids_helper`;
3. gathers and converts the routed F32 activation to Q8_1; and
4. submits the quantized matrix multiplication.

The existing no-bias graph fusion recognizes an adjacent
`{ MUL_MAT_ID, MUL_MAT_ID, GLU }` gate/up group, but the merged dense-FFN Q8_1
reuse path explicitly admits only ordinary `MUL_MAT`. These facts come from
`ggml/src/ggml-cuda/ggml-cuda.cu`, `mmq.cu`, and `mmid.cu` at the exact base.

**[INFERENCE]** An exact gate/up group with the same route IDs and activation
can remove one ID-helper submission and one activation gather/Q8_1 conversion
while retaining both MMQs. The ceiling depends on real graph reachability and
the fraction of prompt time spent in this preparation. Static source cannot
establish either quantity.

## Ranked next candidates

| Rank | Candidate | Likely ceiling | Complexity | Decision at this screen |
|---:|---|---|---|---|
| 1 | Routed-MoE gate/up ID mapping plus Q8_1 preparation reuse | Medium to high for MoE prompt work; no expected ordinary one-token decode effect | Medium | Implement one bounded, default-off slice under ADR-0060 |
| 2 | Fuse an activation producer such as RMS normalization or GLU with Q8_1 preparation | Potentially high by reducing launch and memory traffic | High: numerical, alias, lifetime, and graph-order boundaries | Retain as a later design study |
| 3 | Tune gfx1151 MMQ tile geometry from common prompt/expert shapes | Potentially high | Medium implementation, high measurement burden | Do not choose statically; requires target kernel traces and a shape sweep |
| 4 | Consume packed ROCmFPX Q6 directly instead of its expanded device layout | Medium to high but uncertain compute/bandwidth trade | High kernel and view/offset complexity | Retain as an evidence-gated kernel study |
| 5 | Broaden exact-name Q/K/V graph admission | Low per-hit kernel ceiling; improves reachability only | Medium graph-semantics risk | Wait for real-model reachability evidence |
| 6 | Negotiated RPC completion/events and prompt pipelining | High dual-node system ceiling | Very high protocol, reconnect, ordering, and failure complexity | Keep in its independent distributed-systems lane |

The ranking is comparative engineering judgment (`[INFERENCE]`), not measured
performance. It favors the smallest model-general source seam that can be
made fail-closed and independently tested without target access.

## Distributed boundary

The selected work is rank-local only. A single-node HIP backend may reuse
scratch within its own graph evaluation. In a dual-node RPC deployment, each
worker rank independently evaluates the selector and owns its ID mapping,
Q8_1 pool allocation, stream, counters, and fallback. No scratch pointer,
route mapping, or success state crosses RPC. Split buffers are rejected.

If either rank fails any predicate, that rank executes both legacy
`MUL_MAT_ID` operations independently; the other rank makes its own decision.
The option changes no RPC protocol and has no coordination or recovery
authority. Matched dual-node qualification must bind the exact worker binary
and option state on both ranks, even though correctness does not require both
ranks to take the same local branch.

## Evidence still required

**[OPEN]** Issue #41 prohibits target execution. When that stop gate is
explicitly cleared, promotion requires CPU-reference parity, valid and
malformed routing controls, graph replay, independent per-rank submission
counters, real-model reachability, and matched feature-OFF/ON prompt and TTFT
samples on both Strix Halo machines. Generation must be reported separately.
