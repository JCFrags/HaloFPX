# P06e MiniMax-M2 Q6 replicated shadow-compute canary

Status: **exact-artifact one-layer local/RPC shadow compute qualified; physical expert sharding remains closed**

P06e adds a second strict, default-off gate,
`HALOFPX_MINIMAX_M2_EXPERT_SHADOW_COMPUTE=1`, on top of P06d's explicit
shadow-layer placement gate. When both gates are present, one exact MiniMax-M2
decode layer reuses the authoritative global top-8 expert IDs and normalized
weights. Experts 0 through 95 contribute on the local ROCm branch and experts
96 through 191 contribute on the RPC branch through complementary masks.

Both branches currently read complete replicated `Q6_0_ROCMFPX` expert
tensors. This is deliberate: live qualification found that nonzero expert-axis
views of the RPC duplicate returned a zero peer contribution. P06e therefore
proves routing reuse, masked ownership, real local/RPC Q6 execution, ordered
readback, and equivalence without claiming physical weight sharding or a speed
implementation.

The normal full MoE result remains the only output authority. A CPU-pinned
custom oracle receives the authoritative result plus both branch partials,
requires finite values, NMSE at most `1e-6`, and maximum per-element scaled
error at most `1e-3`, and then copies the authoritative result bit-for-bit into
the residual path. Any mismatch aborts the disposable canary before HTTP
output. Prefill remains authoritative-only. Admission rejects non-default or
embedding graphs, missing P06d placement, alternate topology, wrong geometry,
and wrong tensor types. The one-token shadow path also rejects multiple
sequences or outputs, any active LoRA, and a control vector affecting the
designated layer.

## Exact two-node qualification

Nimo-2 coordinated layer 32 on local `ROCm0`; nimo-1 served `RPC0` at
`10.44.0.1:50053`. The isolated Release build used GCC 16.1.1 with HIP,
Vulkan, and RPC enabled and the WebUI disabled. The exact artifact was revision
`dba517197f2854f3d362529e13abddcdcad6c10b`, file
`saricles-MiniMax-M2.7-REAP-172B-A10B-Q6_0_ROCMFPX_AGENT.gguf`, size
159,873,097,824 bytes, SHA-256
`96506ada918e60ca9a9cfde8a5437790e4453401a6a3e236e3f55e7bac3aaea6`.

The final retained request contained 1,129 prompt tokens and generated 128
tokens. It returned HTTP 200. The response content was byte-identical to the
P06d retained control; the established newline-terminated content SHA-256 was
`a9c38c7f948adcfa8cfab5468ab84cc089b01a34c3f270f1c487a9a5fa74b555`
and the raw content-without-newline SHA-256 was
`3c9bcc5624f4a1d6558c51be89e2443ffba4b2206d10b632c89b7ef64fdd026f`.
Because a failed oracle aborts before response visibility, completing all 128
tokens proves that every decode oracle evaluation in the request passed.

The single correctness run reported 194.56 prompt tokens/s and 15.87
generation tokens/s. The CPU oracle synchronizes every qualified decode and
both ranks execute eight selected slots against replicated weights, so these
numbers are not performance evidence and must not be compared as a candidate
speed result. Matched performance work remains closed until the oracle is
removed from the hot path and rank-local work is compacted.

The strict malformed compute value exited nonzero before tensor loading. The
inherited feature-off and locked L02 controls passed 2/2 on the final source.

## Retained defects and boundary

Three pre-promotion attempts are retained rather than hidden:

1. The first custom oracle was scheduler-assigned to the RPC worker's HIP
   graph, which rejects custom operations. Both disposable processes aborted
   and returned no HTTP output.
2. Pinning the oracle to CPU exposed a real equivalence failure:
   `nmse=0.172685593`, scaled maximum error `0.444621575`, five local and three
   peer selected experts, local L2 `18.5494002`, and peer L2 exactly zero.
3. Making the peer mask the exact complement of the local mask produced the
   same zero peer vector, isolating the defect below mask construction at the
   RPC expert-axis-view boundary.

The accepted implementation therefore uses full replicated tensors and global
expert IDs on each branch, with complementary contribution masks. The RPC
view defect is a recorded next seam, not silently treated as qualified.

P06e does not make the shadow result authoritative, physically shard weights,
compact selected work, prove compute/communication overlap, improve
performance, enable persistence, or alter KV, attention, TurboQuant,
ROCmFPX, HIP, Vulkan, RPC, MTP, or speculative defaults. No donor, GPL
llama-ai, or CachyLLama code, new dependency, model mutation, remote, WebUI,
persistent write, deployment replacement, or reference-clone change entered
the milestone.

The disposable coordinator was stopped before its worker. The known-good
nimo-2 worker and nimo-1 server were restored worker-first; final service and
HTTP-health evidence is sealed with the qualification bundle.
