# P06c MiniMax-M2 Q8 expert-partition equivalence

Status: **representative expert-axis equivalence qualified; runtime remains closed**

Correction from P06d exact-artifact intake: the pinned 160 GB primary model's
expert tensors are `Q6_0_ROCMFPX`. P06c uses a synthetic Q8 tensor with the
same shape and therefore proves partition mechanics for that Q8 backend
variant, not Q6 equivalence for the primary artifact.

P06c proves the first exact-shape computation across the P06a ownership seam.
It is one evaluation-only graph inside `test-backend-ops`, present only when the
existing `HALOFPX_MINIMAX_M2_EXPERT_PARTITION_CANARY` option is explicitly
enabled. That option defaults `OFF`. The test links the P06a component only
into `test-backend-ops`; neither `llama` nor `llama-server` gains a link or
runtime branch.

## Representative graph

The canary uses the pinned MiniMax-M2.7 down-projection shape: one synthetic
Q8_0_ROCMFPX tensor shaped `[1536, 3072, 192]`, top-8 routing, and one token.
P06a creates a four/four cross-rank plan for global experts
`{0, 1, 94, 95, 96, 97, 190, 191}`. The full tensor remains authoritative;
rank 0 and rank 1 receive zero-copy 96-expert views at byte offsets zero and
`96 * weights->nb[2]`.

The unsplit path consumes global IDs. Each rank path consumes P06a's local IDs
and receives the original activation only for slots it owns; every non-owner
slot is all zero, making dummy local expert zero harmless. The two rank outputs
are added and packed beside the unsplit output. Both the ROCm backend and CPU
reference must independently keep unsplit-versus-split NMSE at or below
`1e-6`. Existing NaN and infinity checks remain active.

Weights are deterministic and expert-specific. They are quantized and uploaded
one expert at a time, bounding temporary host memory to one expert instead of a
multi-gigabyte full-precision tensor.

## Focused qualification

Nimo-1 rebuilt `test-backend-ops` in Release mode with GCC 16.1.1, CMake 4.3.4,
HIP enabled, and gfx1151. The final equivalence executable returned one
supported case with no error at the `1e-6` internal NMSE bound. Its SHA-256 is
`cb905ac2a7b5f67f1c17968e32f4cb37a690ba69b0add2d1891a6b05ec963346`;
the final equivalence CSV SHA-256 is
`4382c082c0d4186e1a02b2d3b46484cfdee089487e51646655a4bbebc0218829`.

The inherited four-case P06b exact-shape Q8 roster passed 4/4 during the
initial P06c build, and the unchanged P06a contract executable passed. The
only subsequent source correction removed the ordinary ROCm-versus-CPU term
from this custom oracle and tightened its bound from `5e-4` to `1e-6`; P06b
already owns backend-versus-CPU qualification. The final equivalence test was
then rebuilt and passed. No repeated performance or fault matrix was added.

The selected evidence manifest SHA-256 is
`ba4e16ac9c3359216a1b8fa5664d59310b399e7c0fda914c44bcebd5dbeb34d2`.
The 430,399-byte evidence bundle SHA-256 is
`bbaf4b743447b1a0ebafc7c51222bff608278d40373d70ad47b391bb020dd11a`;
all manifest entries verify.

## Boundary and next step

P06c proves representative down-projection equivalence for two contiguous
expert shards using the authoritative P06a remap and the synthetic Q8 backend
variant. It does not prove Q6 partition equivalence for the primary artifact
and does not yet exercise the
router, gating weights, up/gate projections, activation, selected-expert
reduction, RPC transfer, local/remote overlap, model loading, or server token
generation. It makes no speed claim. P06d subsequently qualified a default-off
exact-model Q6 peer-data placement canary with unchanged layer-owned attention,
KV state, and matched authoritative output. Q6 partition compute and peer-loss
qualification remain closed before performance measurement.

The known-good nimo-1 coordinator and nimo-2 worker were stopped in dependency
order for the isolated GPU run and restored worker-first. Both returned active
with zero restarts, and nimo-1 returned `200 {"status":"ok"}` after the
rollback model finished loading.

No donor code, GPL llama-ai code, CachyLLama code, dependency, WebUI,
persistent write, model mutation, remote, deployment replacement, or
reference-clone change entered this milestone. Generation above 30 tok/s
remains a stretch objective.
