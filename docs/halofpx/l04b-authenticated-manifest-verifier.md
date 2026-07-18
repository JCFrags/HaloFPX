# L04b offline authenticated-manifest verifier

Status: accepted implementation milestone. See the linked L04b review.

L04b extends the accepted L04a bounded structural parser with offline-only
authentication, protected-anchor matching, and compatibility rejection. The
verifier is a separately excluded static target. It is not linked into
`server-context` or `llama-server`, performs no I/O or payload decode, and has
no provider/candidate path.

## Terminal result boundary

The sole positive disposition is `authenticated_unadmitted`. It means one
canonical manifest was authenticated with an exact active key tuple and matched
trusted authority, replay anchor, and compatibility expectations. It is still a
terminal miss. No profile or codec is admitted; the result cannot enumerate or
open an object, construct a candidate, restore state, or become a hit.

All failures remain bounded internal dispositions whose external cache behavior
is an opaque miss. This milestone emits no security event or metric because it
has no runtime or logging integration.

## Exact verification order

1. Run the L04a structural parser over at most 1 MiB. Use the original canonical
   byte spans; never reconstruct the manifest for authentication.
2. Select exactly the supplied `(key_id bytes, key_generation)` record. Unknown,
   revoked, read-disabled, missing, oversized, or different-generation key state
   fails without fallback or scanning.
3. Derive `K_manifest` using the ADR-0003 domain, UTF-8 byte length as `u16be`,
   exact key-ID bytes, store UUID, namespace, and `u64be` key generation.
4. Verify HMAC-SHA-256 over the domain including its NUL plus the exact original
   `manifest-auth-input-v1` DCBOR span. Compare all 32 tag bytes with a fixed-trip
   XOR/OR loop before authority, replay, or compatibility interpretation.
5. Match authenticated store UUID, namespace, and policy epoch to trusted scope
   expectations.
6. Match lineage, key generation, selected generation, predecessor, and the
   domain-separated digest of the complete authenticated manifest to the exact
   protected replay-anchor expectation.
7. Recompute the stored compatibility root from the exact original 16-field
   DCBOR map; then recompute the trusted expected root from its canonical closed
   map and compare the root plus all 16 component digests.
8. Return only `authenticated_unadmitted`.

The policy key is borrowed synchronously and never retained or mutated. The
target-owned wrapper wipes its key blocks, pads, digest buffers, and context
objects through volatile stores. The inherited SHA transform has unwiped local
working words, so stack/compiler/platform remanence remains explicitly
unqualified and blocks live protected-key use.

## Crypto source and provenance

The selected base contains one suitable portable SHA-256 primitive at
`examples/gguf-hash/deps/sha256`. Its source declares Igor Pavlov public-domain
code based on public-domain Crypto++ work. The exact inherited source now builds
once in the excluded `halofpx-context-store-sha256` target; the excluded L04b
target links it and adds a target-owned RFC 2104 HMAC wrapper. This does not copy
a donor unit or add OpenSSL, BCrypt, server, or network linkage.

The static contract pins:

| Inherited base path | Git blob | SHA-256 |
|---|---|---|
| `sha256.c` | `a7a87aeb...` | `EA70A42189B4C798657CB2FB334AD6AFB456245B13D3D36D0FF9FD4F9D9E8F62` |
| `sha256.h` | `21657e66...` | `697B3138AA7590D4C86F332B80E241990674C8344D47171A259B16353309B056` |
| `rotate-bits.h` | `75c4881f...` | `633D6F97ABABF28A562B820FC49C6A8788EA56349568CBE908C051AC032F0685` |

No CachyLLama or GPL llama-ai implementation or documentation entered this
milestone. The direct-cherry-pick roster remains empty and no donor P3 record is
required for an unchanged selected-base public-domain source.

## Synthetic qualification

Target-owned tests use NIST SHA-256 and RFC 4231 HMAC vectors, including SHA
padding boundaries and a key longer than one block. A Python `hashlib` reference
independently fixed golden values for the exact synthetic compatibility root,
manifest-key derivation, manifest tag, and selected-manifest digest.

Negative cases cover null/invalid primitive spans, first/middle/last tag flips,
authenticated body corruption, unknown/revoked/read-disabled/wrong-generation
keys, unavailable key policy, multibyte key IDs, explicitly registered generation
zero, authority mismatch, wrong lineage/generation/digest anchors, internally
corrupt compatibility roots, all 16 expected-component mismatches, a re-signed
changed component, structural truncation, and re-signed descriptor substitution.
Even the last case returns only `authenticated_unadmitted`.

## Gates intentionally still closed

- The key record is caller-supplied test policy, not a protected service key
  registry or authentication authority.
- The fixed-trip tag compare requires Release assembly review on MSVC and
  GCC/Clang before it may protect a trusted hit; portable C++ alone is not a
  formal constant-time proof.
- Platform/compiler key-remanence behavior and service crash handling are not
  qualified.
- No compatibility-component encoder, state profile, codec, payload/object
  reader, filesystem defense, replay-anchor store, provider hook, or writer is
  admitted.
- Persistent reads and writes remain disabled. Feature-off behavior remains the
  compatibility control.
