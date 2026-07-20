# L08d excluded authenticated full-v1 read composition

L08d delivers the smallest safe full-v1 reader composition after L08c. It
expands the authenticated carrier to retain every closed descriptor and
topology fact, then composes manifest authentication, exact trusted admission,
and complete immutable-frame verification into one all-or-nothing synthetic
candidate. It remains memory-only, excluded, and incapable of decoding or
restoring live inference state.

The implementation parent is HaloFPX
`bafaf1c55209b2d3f07a29a44e78b9558d608091`, tree
`1d725e90d02679f748c58995e78bfb55c56286b7`. The locked source authority
remains ROCmFPX `61f2f2d7bc4955e9bca821095ef69125837133b5`, tree
`0a35143f33a7b99a81c824fa8ffd8f743f7ae0dd`.

## Delivered boundary

The full-v1 parser and authenticated carrier now retain codec/schema,
token/boundary, rank/topology, producer, durability, and compatibility facts.
Only `authenticated_unadmitted` exposes the sanitized owned carrier; keys and
tags are never included.

The new `halofpx-context-store-v1-read-only` target is
`STATIC EXCLUDE_FROM_ALL`. It deep-copies one synthetic source record, exact-
matches a separately trusted closed admission, authenticates the manifest,
verifies every ordered frame under explicit per-object and aggregate limits,
and only then constructs one immutable candidate. A bad second object yields a
miss with no partial candidate. Publish is unconditionally disabled and all
reported production capabilities remain closed.

The supplied admission is only a fixture allowlist. It is not protected product
authority, and the isolated `hit` means only "internally verified fixture
candidate." The normal-destruction key wipe is best effort; compiler, allocator,
crash, and process-remanence limits remain behind the production key-custody
gate.

The deterministic two-object fixture pins:

- manifest SHA-256
  `f724dd615d4a5866a655422682d0781eebd0085a25edd587b169f6b8461e4e4b`;
- token-object SHA-256
  `c6a416f1d11eb0e35065f5ddf8912d9270f45e2cd96ba8fa416fd43f1565337c`;
- KV-object SHA-256
  `e1aa2922082eb9433a3907c131fc4fabd26455da2d48189040c25e0da41fa214`.

## Risk-proportionate qualification

Windows MSVC Release passed the parser, authenticator, object verifier, new
composition test, and their four static contracts: 8/8. The new test covers
the golden candidate and payloads, identity miss classes, key/auth/replay
rejection, re-signed unadmitted profile and codec, second-object corruption as
an atomic miss, deep ownership after caller mutation, 128 concurrent lookups,
closed capabilities, and disabled publish. Assertions remain active in Release.

The representative nimo-1 Linux Release build used GCC 16.1.1 with HIP,
Vulkan, curl, and WebUI disabled. The same focused set passed 8/8, and the
feature-off plus L02 contracts passed 2/2. The known-good MiniMax service
remained active and enabled. Local versus nimo-1 hashes for all 14 transferred
source/test files matched exactly. All five immutable reference clones retained
their locked commit/tree and clean worktree.

The retained Linux evidence bundle is
`/var/tmp/halofpx-l08d-v1-read-only-bafaf1c-20260720/halofpx-l08d-v1-read-only-nimo1-20260720.tar.zst`,
12,483 bytes, SHA-256
`d688b6c74d5b65fff14efa2fb8cd89535943bcfe46f8d9e67807ded6749925fa`.
The focused Linux test binary SHA-256 is
`1c59dfed5dc0e8bba22c7f99a205cfe106573c63f92bb6da07da57b1da576193`.

One initial feature-off attempt was not a product failure: the target binary
had not been built in the new test tree. The subsequent server build first
encountered the upstream WebUI provisioning path; reconfiguration with the
required `LLAMA_BUILD_WEBUI=OFF` completed and both contracts passed. No WebUI
asset entered the repository or delivered build.

## Review, rollback, and next gate

This milestone is not a production persistent hit and does not promote L08.
Safe filesystem identity and streaming, real codec/profile admission, semantic
compatibility construction, and protected full-v1 publication remain genuine
production gates. Protected key/anchor sourcing and payload zeroization also
remain closed. Retention and quotas must be built around full-v1 only after
the reader/writer boundary exists; exact-prefix L09 remains later in the
accepted order.

Broader malformed-field Cartesian tests, filesystem faults, crash injection,
distributed recovery, and soak are deferred until a concrete reader/writer
opens those risks. Rollback is one coherent revert; the target has no product
link, install edge, runtime option, or service deployment.

No donor implementation, GPL llama-ai code, CachyLLama transplant, WebUI,
remote, new dependency, or persistent write entered L08d.
