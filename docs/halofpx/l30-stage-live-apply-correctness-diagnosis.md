# L30 stage-to-live-apply correctness diagnosis

Date: 2026-07-23

Base: `8b54091efe456c8222528ec455316afbca8c8562`

Outcome: **PASS — SOURCE-BACKED Q8_0 APPLY CORRECTION**

## Finding and correction

[VERIFIED] L29 authority remains unchanged: capture and validated stage agreed
at 64 components, 2,454,528 bytes, and aggregate
`014a1024f13225a3f7bd7bba6be43dce1106a0354d68b5043f284263cce19bc9`;
live post-apply differed at
`6e9418798c60dc8b6a51ec8c155148ef465a9bc51c7c458295e3666c2141b9a6`.
Coordinator receipts were equal and output diverged at the first token.

[VERIFIED] Source inspection found that commit-live converted component bytes
to a `ggml_view_1d()` element count by dividing by the 34-byte Q8_0 type size.
For the measured 1,088-byte synthetic component, the old path requested 32
scalar elements and therefore described only 34 bytes. The corrected path
multiplies the block count by Q8_0's 32-element block size, producing 1,024
scalar elements and the full 1,088-byte view. Checked divisibility,
multiplication, nonzero, and signed-range guards fail closed.

This establishes a source-backed block-geometry defect. It does not establish
aliasing, asynchronous completion, or any additional primary-model semantic
defect.

## Bounded authenticated diagnostics

[VERIFIED] With `HALOFPX_STATE_DIAGNOSTICS=1`, capture, validated stage, and
immediate live-post-apply now emit one bounded record per component keyed by
authenticated ordinal, kind, type, dimensions, strides, view offset, label
digest, and byte size. Records also carry content hashes and normalized
buffer-relative ranges. Deterministic leaf/Merkle hashes and a control-key
authenticated summary permit offline integrity verification and deterministic
first-divergence reporting without logging state bytes or raw pointers.

[VERIFIED] Focused tests cover exact identities, views, non-contiguous stride
metadata, overlap refusal, first-content divergence, an incomplete synthetic
asynchronous observation followed by completion, and leaf/Merkle/tag
tampering. Backend source audit confirmed that the applicable ROCm copy and
readback paths synchronize before returning; no synchronization correction
was made.

## Qualification

[MEASURED] An isolated real RPC Q8_0 fixture captured, staged, and applied one
1,024-element / 1,088-byte component exactly. All three descriptor/content
aggregates were
`bbd3d3305a5c3d7fd108de4c5b033a11ba0183ce364a9bed3331ef11a432cfea`;
the content digest was
`9a401342a93b10cf363fe10a4fbcb58df4fa7bbd5b70b48f50cc0a0af4ce155b`.
The offline analyzer authenticated all summaries and reported no component
mismatch.

[MEASURED] A separate view fixture applied the exact 32-byte range at offset
16. Capture, stage, and apply aggregates were
`8dcb3ef01d2d6036d7b62ed6b308d095b9956bbebccba13f9abe64bf87861024`;
the authenticated report contained no mismatch.

[MEASURED] The accepted 19,077,344-byte stories15M disposable fixture used the
honest two-residency worker-restart lifecycle, a 1,129-token prompt, 1,128-token
boundary, and maximum decode chunk 512. Capture, stage, and apply each reported
1,156 components, 5,197,824 bytes, and aggregate
`ed62ec5b04cc53ce870ddd6df1d8eefc10a0e4f44e2a699deb879ebf4462fdbc`.
Capture and restored token were both 4245, decoded/token hashes were exact,
coordinator receipt digests agreed, and both state windows contained zero
legacy `GET_TENSOR`/`SET_TENSOR` operations.

[VERIFIED] Focused Python qualification passed 84 of 84 tests. The isolated
ROCm RPC server and C++ fixture built successfully from the recorded source
archive and exact binaries. No primary artifact was accessed.

## Production and cleanup

[VERIFIED] Production was never mutated. At closeout, nimo-2's system worker
unit remained active on PID 1454894 / port 50052 with `NRestarts=0`.
Nimo-1's system coordinator unit remained active on PID 2283857 / port 8081,
served HTTP 200, and had `NRestarts=0`.

[VERIFIED] All L30 and reused L28 disposable units, ports, keys, state roots,
source/build roots, rendezvous paths, and local archives were removed.

The immutable raw evidence is under
`C:\Users\britt\Documents\Custom_Inference_Project\sources\halofpx\l30-stage-live-apply-20260723`.
Its final file count, byte count, and canonical tree hash are frozen in the
receipt after independent review.

The Q8 worker log intentionally retains an initial stopped pre-run before the
accepted successful invocation; the authenticated component report identifies
the successful invocation's result.

## Boundary

L30 is a no-production, no-primary-load diagnosis and narrow default-off
correction. It does not prove a corrected primary restore, authorize another
primary experiment, promote the cache, or open L31.
