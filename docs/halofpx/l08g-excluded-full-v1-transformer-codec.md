# L08g excluded full-v1 transformer snapshot codec

L08g closes the missing memory semantic bridge between the admitted L07
transformer snapshot and the authenticated full-v1 L08d candidate. It is
target-native, excluded from normal builds, and has no filesystem, publication,
server, or live-restore edge.

The implementation parent is HaloFPX
`000a9476181fcc9f06ca106d61656e2211f22319`, tree
`6841e380eedeed6c1102c1a0ebf60dec2b9253fe`. The locked ROCmFPX authority
remains commit `61f2f2d7bc4955e9bca821095ef69125837133b5`, tree
`0a35143f33a7b99a81c824fa8ffd8f743f7ae0dd`.

## Delivered boundary

The encoder admits only the existing target-only world-one/rank-zero
memoryless-greedy transformer profile. It encodes exact int32 big-endian tokens
and opaque `llama_state_seq_*_ext` bytes as two versioned frames, constructs the
canonical 15-field full-v1 manifest, signs it with the existing manifest KDF
and HMAC contract, and passes the result through the independent parser and
authenticator before exposing it.

The decoder begins only from an authenticated, frame-verified L08d candidate.
It closes the profile, codec, schema, topology, ownership, compatibility,
boundary, token-digest, exact-token, and state-size contracts before returning
an owned L07 snapshot. It never mutates live inference state.

Independent review rejected the first draft for two P2 implementation-order
defects. Exact frame limits were initially checked after frame allocation, and
the derived manifest key existed across a later allocation. The final code
precomputes and admits both exact frame sizes before payload allocation and
fully constructs the authentication message before deriving the key. A focused
frame-cap regression was added. Re-review accepted both corrections with no
remaining P1/P2.

## Risk-proportionate qualification

Windows MSVC Release passed the new codec, its static isolation contract, the
inherited L07 transformer codec, the L08d full-v1 reader, and the L08d static
contract: 5/5. The focused test covers the exact round trip, wrong profile and
codec rejection, second-frame corruption as an atomic provider miss, encode
and decode limits, and empty output on failure.

The representative nimo-1 Linux Release CPU build used GCC 16.1.1 with HIP,
Vulkan, curl, and WebUI disabled. Feature-off and L02 controls plus the L07,
L08d, and L08g focused tests/contracts passed 7/7. The known-good MiniMax
service remained active and enabled. All six transferred source/test files
matched their local SHA-256 values.

The retained Linux evidence bundle is
`/var/tmp/halofpx-l08e-linux-reader-9907b90-20260720/halofpx-l08g-v1-transformer-codec-nimo1-20260720.tar.zst`,
3,998 bytes, SHA-256
`f4349f510628649d9462128d5c8f425c5a3c10b890692707f77b033986babda2`.
The focused test binary SHA-256 is
`41f8ad0b62a2c7e2c0257a7d04f15a55c73ed0ac1218eac57ceb715d5879a2e2`;
the feature-off server SHA-256 is
`f25fb23c47e8480136dd3fc0dffdf5a94adb732145a82a05a66996089d6f8bd7`.

## Boundary and next gate

The product admission registries remain empty. L08g does not acquire protected
keys, publish persistent material, advance an anchor, reconcile attempts,
restore a live slot, or link into the server. Broader decoder permutations,
allocator-remanence hardening, crash/storage-exhaustion matrices, retention,
administration, distributed recovery, and soak are deferred until the
protected writer or product edge makes them material.

The next milestone is the smallest generation-one protected full-v1 material
and attempt authority that can support a default-off miss/write/restart/hit
server canary. No donor implementation, GPL llama-ai code, CachyLLama code,
WebUI, remote, dependency, model, or service deployment entered L08g.
