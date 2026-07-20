# L08c closed compatibility-component authority

L08c replaces the protected canary's opaque compatibility-root input with a
default-off, target-native builder over the exact closed 16-component registry
from ADR-0003. It is the smallest safe step toward canonical compatibility
construction: it binds the trusted component set and ordering used by the
protected canary, but it does not yet construct or validate each component's
semantic preimage.

The implementation parent is HaloFPX
`28eaba944250d3b917fc7006a1ffd07e9aed6ea4`, tree
`82e9f793b56f18911f001469395b79b300063978`. The locked source authority
remains ROCmFPX `61f2f2d7bc4955e9bca821095ef69125837133b5`, tree
`0a35143f33a7b99a81c824fa8ffd8f743f7ae0dd`.

## Delivered boundary

The new `HALOFPX_CONTEXT_STORE_COMPONENT_AUTHORITY` build gate defaults to
`OFF` and requires both existing canary gates. With it enabled,
`protected-rw-canary` rejects `--halofpx-context-store-compatibility-root` and
requires one ordered comma-separated list containing exactly the 16 registered
`LABEL=HEX64` component digests. Unknown, missing, duplicate, misordered, zero,
non-lowercase, or malformed inputs fail configuration before store access.
`direct-rw` retains its prior opaque-root input and rejects the component list.

The builder retains the exact 16 component digests and computes:

`SHA-256("halofpx.compat.v1\0" || DCBOR({0: digest0, ..., 15: digest15}))`

The deterministic body is a 16-entry integer-key map; each value is exactly a
32-byte byte string. The checked-in independent golden vector is 579 bytes and
has root
`25d572e33e6a118a2fd1785ccea2a64164b1d371223b62e7c7fa87088ee35851`.

The server also closes state-profile gaps found during review. It admits only
an exact memoryless-greedy sampler shape, rejects control vectors, KV metadata
overrides, and active tensor-placement overrides, and retains the existing
recurrent, hybrid, multimodal, adapter, draft, MTP, and speculative rejections.
ROCmFPX pads the tensor-override vector with null terminators even when no
override is active; the final predicate rejects actual non-null overrides
rather than that inert parser storage.

The protected canary's test authority hashes explicit synthetic facts for each
registered component. This is trustworthy as a test fixture only. The engine
does not yet collect GGUF arrays, tokenizer merge bytes, rendered-template
state, all runtime ABI facts, or placement details into component preimages,
so L08c is not the full ADR-0003 manifest builder and does not promote
persistent writes.

## Representative Linux qualification

The final nimo-1 Release CPU build used GCC 16.1.1 with WebUI, HIP, Vulkan, and
curl disabled and all three canary gates enabled. Seven focused contracts
passed: feature-off, L02, compatibility-v1, scope, transformer codec, direct
provider, and protected provider. The all-off build passed feature-off and L02
and exposed zero HaloFPX context-store options.

The final live protected test passed 1/1 in 0.75 seconds with a 19,077,344-byte
Stories 15M Q4_0 fixture, SHA-256
`66967fbece6dbe97886593fdbb73589584927e29119ec31f08090732d1861739`.
It observed:

- initial authenticated restore: `miss-not-found`;
- protected write followed by restart: exact-principal `hit`;
- one changed tokenizer component under the same session: `miss-not-found`;
- restoration after returning to the original 16 components: `hit`;
- wrong principal: `miss-not-found`;
- corrupted anchor: `miss-corrupt`, then equal cold recomputation; and
- unsupported profile and unexpected-root startup: writes disabled while cold
  inference remained available.

The cold continuation digest remained
`d4befa4c08b0bdd9023bfa965064be3c3a8eda8804174fc74c286b2a66710860`.
The enabled server and common-library SHA-256 values are respectively
`cb03575fe8c95774893e523c754756e129a136806593616c4c49579abf69b766`
and `e66a31921ddde97f25e3532c9a587e16316f950201bed6152b3dc530bfe61c3a`.

Two early qualification attempts identified and corrected real contract
issues: automatic `--fit on` created undeclared placement overrides, so the
canary now fixes `--fit off`; and the padded null override array required an
active-entry predicate. A separate inherited slot-save smoke rerun was stopped
after the current upstream preset fetch failed at TLS setup. The identical
smoke passed at L08b, and no slot implementation changed in L08c. Per the
owner's risk-proportionate testing direction, broader repetition was deferred.

The sanitized nimo-1 evidence bundle is
`/var/tmp/halofpx-l08c-closed-component-authority-final-20260720-a3/halofpx-l08c-closed-component-authority-nimo1-20260720.tar.zst`,
30,928 bytes, SHA-256
`2e63fcee4bb4023428e64e7d9af579e33d081de3105ac85aeccd88d08f3e856d`.
It excludes the authority key, store objects, anchors, model, and user data.

## Review, rollback, and open gates

Rollback is an all-off rebuild or one coherent revert. The protected mode
cannot accept an opaque compatibility root when component authority is built,
and every feature remains default-off.

This milestone does not supply target-owned semantic encoders for the 16
component preimages, a complete authenticated v1 manifest, generation
advancement, production key custody, retention/eviction administration,
distributed ownership, two-node recovery, soak qualification, or final
zero-regression evidence. Exhaustive malformed-CLI, filesystem, crash,
disk-full, concurrency, upgrade, and topology permutations remain deferred to
the promotion gates or a concrete defect hypothesis.

No donor implementation, GPL llama-ai code, CachyLLama transplant, new
dependency, WebUI, remote, model, or service deployment entered this change.

Independent review accepted this boundary with no P1/P2 blocker after the
wording correction above. Acceptance is limited to the default-off
generation-one laboratory canary and does not promote L08 or production
persistence.
