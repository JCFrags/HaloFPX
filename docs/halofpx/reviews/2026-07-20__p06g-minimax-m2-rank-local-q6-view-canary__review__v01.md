# P06g MiniMax-M2 rank-local Q6 view canary independent review

Date: 2026-07-20

Verdict: **accept for commit**

No P0, P1, or P2 blocking finding remains.

The RPC graph reconstruction resolves the server-side view source, requires
the view and source to use the same registered buffer, discards the derived
client pointer, and reruns backend view initialization. Failure propagates
without executing an incompletely reconstructed view. This corrects the
backend-layout authority boundary without enabling shadow compute or
persistence.

The MiniMax-M2 graph uses independent float casts before the in-place clamp
operations, masks out non-owner slots, remaps peer IDs by 96, and constructs
96-expert views at the expected expert-axis packed offset. The full local MoE
result remains authoritative and both feature gates remain default-off. The
focused Q6 oracle now requires nonzero reference energy before accepting exact
direct-HIP or RPC equivalence.

The recorded source hashes match the reviewed files. The retained node
manifests and bundles verify, the focused direct and RPC tests produced
nonzero exact outputs, and the exact 160 GB model canary returned HTTP 200 with
the same content hash as P06d/P06e. Known-good services were restored with
zero restarts and HTTP health. The P06f correction is explicit and preserves
the still-valid direct-HIP finding.

Non-blocking debt: the generic RPC correction has focused Q6 and exact-model
end-to-end coverage, not a broad cross-backend view matrix. Under the current
risk-proportionate testing direction, that matrix remains deferred unless a
concrete defect justifies reopening it.
