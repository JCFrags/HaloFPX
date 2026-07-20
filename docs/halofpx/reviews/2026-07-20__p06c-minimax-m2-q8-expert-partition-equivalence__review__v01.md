# P06c MiniMax-M2 Q8 expert-partition equivalence independent review

Subsequent P06d exact-artifact intake corrected applicability: the pinned
artifact's expert tensors are Q6_0_ROCMFPX. This review covers synthetic Q8
partition mechanics at the pinned shape; it does not prove primary-artifact
Q6 partition equivalence.

Status: **ACCEPT; no P1/P2 correction.**

The review confirmed correct full-versus-two-rank graph semantics: contiguous
expert-axis views, P06a global-to-local remapping, zeroed non-owner
activations, additive reconstruction, and packed output halves aligned with the
custom oracle. Deterministic one-expert-at-a-time Q8 quantization is bounded,
and byte strides and uploads match the tensor layout.

The oracle independently checks ROCm and CPU internal partition equivalence at
NMSE no greater than `1e-6`; P06b separately owns backend-versus-CPU kernel
qualification. The canary remains default-off and links only into the gated
test executable, leaving runtime binaries unchanged.

Parent commit/tree, receipt source hashes, every evidence-manifest entry, and
the bundle size/hash reconcile. Nimo-1 independently returned
`200 {"status":"ok"}` on port 8081; both rollback services are active with zero
restarts.

Claims correctly exclude router and gating behavior, a full MoE layer, RPC,
runtime placement, performance, and persistence. No remote, donor or GPL code,
dependency, deployment replacement, model mutation, WebUI, persistent write,
or reference-clone mutation entered the milestone.
