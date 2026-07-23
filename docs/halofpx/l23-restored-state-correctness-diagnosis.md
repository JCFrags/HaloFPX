# L23 restored-state correctness diagnosis

Date: 2026-07-23

Base: `4d2821b3a318d2d38f93a30aa2f3a2263cc4d01d`

Outcome: **BLOCKED — PRIMARY-SPECIFIC DISCRIMINATING EVIDENCE REQUIRED**

## Earliest established divergence

[VERIFIED] The immutable L22 evidence places the first observable failure at
the first token decoded after restore. Capture, clean cold, both cold
fallbacks, and mode-off produced token 21549 first and suffix SHA-256
`d8c8822f2ad7951dc363056b4c165e6696ad6066a41d68b1a99097526012b6d9`.
The restart-restored context produced token 9283 first and suffix SHA-256
`d32acac3fc1d2ac80ee33ad8cf66192112200b7b93ed88e520c7c56660a71470`.
This is not late generation drift.

L22 recorded 64 worker components and 2,454,528 worker bytes at capture,
READY, and apply; 2,301,688 coordinator-local bytes; and 15,048 control bytes.
It also proved zero legacy `GET_TENSOR`/`SET_TENSOR` state-page operations.
Those receipts authenticate descriptors and the stored object, but L22 did
not retain content digests at worker capture, stage, and live post-apply
boundaries. Therefore the immutable evidence cannot distinguish a worker
apply/layout error from missing or semantically incomplete coordinator-local,
control, KV metadata, recurrent/non-KV, or model-specific state.

## Source audit and diagnostic seam

[VERIFIED] The audited source paths were the coordinator state split and
restore flow in `src/llama-context.cpp` and the worker object capture, stage,
and commit-live flow in `ggml/src/ggml-rpc/ggml-rpc.cpp`. Worker descriptors
bind ordinal, kind, type, dimensions, strides, view offset, tensor-name
digest, component size, rank/world/placement identity, token boundary,
compatibility root, checkpoint/generation, and attempt identity. Object stage
validates the complete immutable object and every component digest before
loading disposable staging buffers. Commit copies every staged component to
the matched live tensor.

[VERIFIED] No completeness, ordering, offset, stride, type, or byte-count
defect was proven by this audit. In particular, the source intentionally
separates coordinator control bytes, coordinator-local tensor bytes, and
worker-owned tensor bytes. Q8_0 K/V, flash attention, MiniMax
architecture/recurrent state, and restart-dependent graph/layout remain
hypotheses, not findings.

A compile- and runtime-default-off diagnostic seam now hashes normalized
worker descriptors plus component content at capture, validated stage, and
live post-apply. It activates only for exact
`HALOFPX_STATE_DIAGNOSTICS=1`, emits bounded digests and counts, transfers no
state bytes over the control plane, and fails the diagnostic apply if live
hashing cannot complete. The disposable canary also emits authenticated
control, coordinator-local, and component-manifest receipt digests.

## Disposable qualification

[MEASURED] The accepted local 15M fixture, SHA-256
`66967fbece6dbe97886593fdbb73589584927e29119ec31f08090732d1861739`,
ran the bounded 1,129-token/1,128-boundary three-residency lifecycle across an
isolated two-host RPC setup. Because that fixture cannot represent Q8_0 KV,
the previously authorized fixture-only F16 KV and flash-attention-off tuple
was used. It is not representative of primary KV/kernel behavior.

Capture, clean cold, true worker-restart restore, missing-object fallback,
plan-mismatch fallback, and mode-off cold all produced first token 4245,
token SHA-256
`1c337bc56fc5eeba46dd328c79c2c6240fd9bcb8d9d1d9aa5b30cd63e04dc6ce`,
and text SHA-256
`2819884398780985d79d86a20376e87238f0864a0b2825c58d5fc6631554bcf5`.
The worker object contained 1,156 components and 5,197,824 state bytes.
Capture, stage, and post-apply each produced the same aggregate digest:
`ed62ec5b04cc53ce870ddd6df1d8eefc10a0e4f44e2a699deb879ebf4462fdbc`.
The authenticated control, local, and manifest digests were respectively
`74994f73a5b5e972d9a57cf897a8869871fe83844b35909d51f57ae07de196e4`,
`dd773b55782dff7623889a9df752e389b5bee09f76d9b423a716db8aecb3b9ee`,
and
`317230332505f5039715f6ed2219eecab6ffd78d5fc0332030c0f6e997fd30d1`.
State windows again contained zero legacy state-page GET/SET operations.

The immutable fixture evidence root is
`C:\Users\britt\Documents\Custom_Inference_Project\sources\halofpx\l23-restored-state-20260723\fixture-diagnostics-v1`.
Its 31 files total 1,190,842 bytes and have canonical
relative-path-plus-content tree SHA-256
`d9f4d4c642bb9c8803028e40b652a8ac709eb9df66c9a51f57b1ae0857b6d0f0`.

## Precise blocker and next experiment

[OPEN] The disposable result proves the diagnostic seam and ordinary RPC
capture/stage/apply lifecycle, but it cannot discriminate the L22 failure
because it changes architecture, KV type, and flash-attention path. No
source-backed root cause is established.

The smallest single discriminating experiment is one separately authorized
primary-model capture and true worker-restart restore, with diagnostics
enabled and no cold/fallback/performance matrix. It must retain the three
worker aggregate digests, authenticated coordinator receipt digests, exact
first generated token, and suffix hash. A capture/stage mismatch identifies
object staging; stage/apply mismatch identifies live worker application or
restart layout. Equal worker digests with a first-token mismatch narrows the
remaining fault to coordinator-local/control metadata completeness or
primary-specific state semantics. L23 does not authorize this experiment.

## Production and boundary

[VERIFIED] Production was never mutated. At closeout nimo-2 worker PID
1396163 listened on 50052, nimo-1 coordinator PID 2213675 listened on 8081
and returned HTTP 200, and both units reported `NRestarts=0`. All L23
disposable roots, keys, builds, units, and port 50183 were removed. The cache
remains default-off. No primary artifact was read or loaded.
