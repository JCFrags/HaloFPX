# L08g excluded full-v1 transformer codec independent review v01

Verdict: **ACCEPT** for the default-excluded memory-only L08g boundary after
two P2 corrections. No P1/P2 finding remains.

## Blocking findings corrected

The first implementation allocated the token payload and complete frames before
checking the declared frame limit. It also derived the manifest key before an
allocating authentication-message construction. Review required exact frame
size admission before those allocations and elimination of every throwing
operation from the derived-key lifetime.

The final implementation:

- computes both exact frame sizes with checked arithmetic and rejects them
  before compatibility, payload, or frame allocation;
- constructs the complete authentication message before key derivation;
- wipes the derived-key output on derivation failure and immediately after the
  non-throwing HMAC operation; and
- adds a focused frame-limit regression proving empty encoded output.

Focused Release rebuild and tests passed after correction. Re-review accepted
both fixes.

## Review conclusions

- The full-v1 manifest layout, compatibility domain, manifest KDF,
  authentication input, object-frame identity, and parser/authenticator
  round-trip match the target-owned contracts.
- Encoding is closed to the L07 target-only world-one/rank-zero
  memoryless-greedy transformer snapshot and exact two-frame roster.
- Decoding begins from an authenticated L08d candidate and exact-checks the
  profile, codec, schema, rank, ownership, compatibility, boundaries, token
  digest, and every expected token before returning a complete owned snapshot.
- Failures expose no partial snapshot and no live restore occurs.
- The target is `STATIC EXCLUDE_FROM_ALL`; there is no filesystem,
  publication, server, install, donor, or service edge.
- The product codec/profile registries remain empty. The L08g IDs are excluded
  implementation contracts, not production admission.
- The implementation is target-native and adds no GPL llama-ai code,
  CachyLLama transplant, donor dependency, WebUI, remote, or provenance duty.

## P3 and deferred boundary

The codec publicly depends on the L07 library, which also contains restore
APIs, although L08g itself calls none. Documentation therefore claims no
live-restore call or server edge rather than an absence of those symbols from
all dependencies. Broader decoder rejection permutations and opaque payload
allocator-remanence hardening remain deferred to the product integration gate.
