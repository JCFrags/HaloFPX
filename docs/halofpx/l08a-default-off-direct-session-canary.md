# L08a default-off direct-session laboratory canary

L08a is the smallest usable, target-native persistent-state demonstration in
HaloFPX. On Linux it proves an authenticated private miss, an immutable write,
a process restart, an exact-session laboratory hit, deterministic continuation,
and safe cold recomputation after corruption. It is deliberately **not** L08
promotion and is not production persistence.

The implementation parent is HaloFPX
`d0deef150d5406aa3ece6fc071abf295fc992434`, tree
`041ed734314569841ad881a4d3817c7deae6eab`. The locked source authority remains
ROCmFPX `61f2f2d7bc4955e9bca821095ef69125837133b5`, tree
`0a35143f33a7b99a81c824fa8ffd8f743f7ae0dd`.

## Delivered seam

The server canary requires both build-time
`HALOFPX_CONTEXT_STORE_CANARY=ON` and the explicit runtime mode
`--halofpx-context-store-mode direct-rw`. The build option is `OFF` by default.
An off build has no cache CLI surface and does not link the provider, scope, or
state-codec targets.

The only operations are authenticated explicit slot actions using an opaque
lowercase 256-bit session ID. The route admits no enumeration, prefix matching,
automatic lookup, anonymous access, cross-principal reuse, legacy format, or
shared scope. The exact ADR-0002 namespace derivation uses a keyed,
domain-separated canonical DCBOR preimage binding policy-key ID,
authentication issuer, principal bytes, security domain, policy epoch, private
scope class, and compatibility root. A fixed golden vector prevents contract
drift.

Codec `transformer-sequence-v1` uses the target sequence-state API and admits
only world-size one, rank zero, transformer target state with an exact
compatibility identity and token sequence. Publication requires native content
output, no parser, tool, grammar, generation prompt, adapter, speculative/MTP,
recurrent, or stateful sampler behavior. Unsupported state is rejected before
publication.

The Linux `HFPXLD01` laboratory provider authenticates a fixed-size manifest
and its exact scope, session, compatibility root, lengths, token count, and
payload hashes. It pins root owner/device/mount/mode, rejects symlinks and extra
layout, enforces one writer, quota, reserve, entry and decode bounds, writes and
synchronizes an immutable staging object, and uses
`renameat2(RENAME_NOREPLACE)`. An existing destination is authenticated and
classified before capacity rejection: an equal retry is idempotent; an unequal
session collision is `conflict`, never success or overwrite. A corrupt object is a miss.
An unusable store at startup disables the canary and leaves ordinary inference
healthy and cold.

## Representative qualification

The final nimo-1 Release qualification used CPU-only canary and feature-off
builds from the same source, CMake configuration, and compiler environment.
The focused set passed 5/5: feature-off contract, locked L02 contracts, exact
scope, transformer codec, and Linux direct-provider tests. The inherited slot
save/restore smoke passed 1/1 with one unrelated case deselected. The off build
passed its feature-off and L02 controls and exposed zero
`halofpx-context-store` help matches.

The opt-in server test passed 1/1 and retained an exact runtime/library/model
compatibility record. It observed:

- first authenticated restore: `miss-not-found`;
- publish followed by process restart: exact-principal `hit`;
- the same session under a second principal: `miss-not-found`;
- cold and restored greedy continuations: equal content and token output;
- corrupted state: `miss-corrupt`, followed by equal cold recomputation;
- tool-parser publication: rejected as outside the admitted profile; and
- unexpected root entry on restart: provider rejected while the server stayed
  healthy and produced the same cold continuation.

The sanitized evidence bundle is retained on nimo-1 at
`/var/tmp/halofpx-l08a-canary-evidence-20260719-v3/halofpx-l08a-canary-nimo1-20260719.tar.zst`,
7,507 bytes, SHA-256
`fdef5e5e2f61f6975341db18b913b41af23f2305fcb937f0c49dc056f7605409`.
It contains no authority key or store object.

## Closed claims and deferred gates

The direct format has no separately protected anchor, generation, replay
high-water mark, or L05 response-loss reconciliation carrier. A power loss
after rename but before destination-directory synchronization has
filesystem-dependent visibility, and a copied authenticated object can be
replayed at the same path. Therefore an observed restart result is only a
private disposable-root laboratory hit. L08 and persistent-write enablement
remain closed until the ADR-0004 through ADR-0026 authority is integrated and
its essential crash boundaries qualify.

Per owner steering, exhaustive crash-point, filesystem, hostile-length,
concurrency, disk-full, stale-lock, upgrade/coexistence, multi-tenant, and
two-node permutations are deferred to the protected-anchor promotion lane or a
specific defect hypothesis. This milestone makes no performance,
zero-regression, HIP/Vulkan, ROCmFPX-weight, distributed-state, retention,
production-key-custody, deployment, or service-change claim.

No donor implementation, GPL llama-ai code, CachyLLama transplant, new
dependency, WebUI, remote, model file, or persistent cache object entered the
repository. Rollback is the removal/revert of the canary-only source, tests,
routes, arguments, and build option; feature-off remains the compatibility
control. The independent adversarial review accepts this restricted L08a
laboratory milestone and explicitly does not promote L08 or production
persistence.
