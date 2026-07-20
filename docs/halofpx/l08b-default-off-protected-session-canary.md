# L08b default-off generation-one protected-session canary

L08b adds the smallest protected publication boundary above L08a. On Linux it
proves an authenticated private miss, immutable direct material, a separately
authenticated generation-one anchor, process restart, exact-session hit, and
safe cold recomputation after anchor corruption. It remains a disposable
laboratory canary below L08 and is not production persistence.

The implementation parent is HaloFPX
`d028cbcbb067bdbdb1656b8ef9d1a0fbeba46f38`, tree
`e3954cf375bf7984578a32b743b9bdd122a99bff`. The locked source authority
remains ROCmFPX `61f2f2d7bc4955e9bca821095ef69125837133b5`, tree
`0a35143f33a7b99a81c824fa8ffd8f743f7ae0dd`.

## Delivered boundary

The product path requires both `HALOFPX_CONTEXT_STORE_CANARY=ON` and
`HALOFPX_CONTEXT_STORE_PROTECTED_CANARY=ON`, plus explicit runtime mode
`protected-rw-canary`. Both compile-time options default to `OFF`. When the
protected gate is off, its runtime mode, anchor-root argument, UUID argument,
provider linkage, and help text are absent. `direct-rw` remains the unchanged
L08a control.

The operator supplies distinct canonical owner-only data and anchor roots, an
exact owner-only 32-byte key file, and a stable lowercase 128-bit store UUID.
The raw key derives three UUID-bound roots: scope, direct-manifest, and anchor.
Only those roots survive initialization. Per-namespace direct and anchor keys
bind exact key IDs and generation one and are wiped after each request.

The direct provider now exposes an owned receipt containing the exact 228-byte
`HFPXLD01` manifest, its canary-specific selected digest, and exact
scope/session/compatibility identities. Manifest-only inspection releases no
payload. Authorized load reopens and reauthenticates the manifest and requires
complete receipt equality before reading and hashing payloads. Legacy L08a
lookup and publication remain wrappers over the same seam with their original
authority.

The protected provider writes a canary-owned byte-exact ADR-0008 envelope at
`<anchor-root>/<namespace>/<session>.anchor`. Its product encoder admits only
policy epoch one, authority epoch one, both key generations one, object
generation one, a null predecessor, the fixed anchor key ID, nonzero exact
identities, and an exact 32-byte key. Verification regenerates the sole
canonical envelope and compares its complete size and every byte. The server
does not link the excluded L05 anchor codec, publication coordinator,
bootstrap, registry, simulator, or synthetic backend.

Publication synchronizes direct material first, stages and synchronizes the
anchor, publishes without replacement, synchronizes the parent, and reopens
both anchor and direct receipt before acknowledgement. An exact-present
ambiguous create can become `recovered-durable` only after a fresh successful
parent synchronization and a second exact reauthentication. Conclusive
absence is `unreachable`; malformed or mismatched observation quarantines the
lineage. Equal collision is idempotent only after both exact anchor and exact
authenticated direct receipt revalidation.

## Representative Linux qualification

The final nimo-1 Release build used CPU, GCC 16.1.1, WebUI off, HIP off,
Vulkan off, curl off, and both canary build gates on. Seven focused tests
passed: feature-off contract, locked L02 contracts, exact scope, transformer
codec, direct receipt provider, canary-owned anchor codec, and protected Linux
provider. The inherited slot save/restore smoke passed 1/1 with one unrelated
case deselected.

The protected provider test covers unanchored material, restart hit, complete
payload equality, corrupted anchor rejection, unequal direct conflict,
conclusive-absence ambiguity, and exact-present durable recovery. The final
opt-in server test passed 1/1 in 0.64 seconds and observed:

- first authenticated restore: `miss-not-found`;
- protected publish followed by process restart: exact-principal `hit`;
- the same session under another principal: `miss-not-found`;
- cold and restored greedy continuations: equal content and tokens;
- corrupted protected anchor: `miss-corrupt`, followed by equal cold
  recomputation;
- tool-parser publication: rejected outside the admitted profile; and
- unexpected direct-root data at restart: provider disabled while the server
  remained healthy and produced the same cold continuation.

The matched compatibility root was
`e676c73f4ee112bd39822c08ffbf47e4d8806f385b9d279c9a2d31f99f644f6f`;
the cold continuation digest remained
`d4befa4c08b0bdd9023bfa965064be3c3a8eda8804174fc74c286b2a66710860`.
The final server SHA-256 is
`801f9c58c88db8929450932c689006241a72d0238e7f3ffa0ec75a9ec767106f`.

Feature-off controls separately rebuilt base-on/protected-off and all-off
servers. Both exposed zero protected options; the all-off server exposed zero
HaloFPX context-store options and passed feature-off plus L02 contracts. The
final protected binary contained protected provider/codec symbols and zero
excluded L05 product symbols.

The sanitized nimo-1 evidence bundle is
`/var/tmp/halofpx-l08b-protected-evidence-final-20260720/halofpx-l08b-protected-canary-nimo1-20260720.tar.zst`,
7,570 bytes, SHA-256
`3925f128d5fa24f76763393edb2ed7d1b03bb05df408763b5a611808e2cfeca1`.
It excludes the authority key, direct objects, anchors, and model.

## Review, rollback, and open gates

Independent review found one P2: the low-probability `renameat2` `EEXIST`
collision path initially revalidated the exact anchor but not the direct
receipt after collision. The correction reopens and authenticates the direct
manifest and requires complete receipt equality before `already-exists`;
mismatch quarantines. The reviewer then accepted the milestone and confirmed
the product link graph excludes every L05 backend.

This milestone does not close full ADR-0004 or L08. It has no canonical
server-produced full-v1 manifest, generation advancement, persistent CAS and
attempt fencing, retention/eviction administration, power-loss matrix,
distributed ownership, two-node recovery, production key custody, or final
zero-regression evidence. Per owner steering, exhaustive filesystem, crash,
disk-full, concurrency, upgrade/coexistence, and two-node permutations remain
deferred until a concrete defect or promotion gate requires them.

No donor implementation, GPL llama-ai code, CachyLLama transplant, new
dependency, WebUI, remote, model, or service deployment entered this change.
Rollback is an all-off build or one coherent revert. Persistent writes remain
disabled by default.
