# ADR-0009: authenticated anchor carrier and ordinary transition

- Status: accepted for the disabled offline L05f coordinator
- Date: 2026-07-18

## Decision

Only the L05e encoder or verifier can construct a valid owned
`context_store_authenticated_anchor`. The carrier privately owns the exact
canonical envelope, parsed body, anchor-authentication key tuple, and
domain-separated envelope digest. It retains no key material. Default instances
are invalid, and rejected codec results expose no carrier.

For ordinary-transition continuity, the carrier also holds
`HMAC-SHA-256(K_anchor, "halofpx.anchor-authority-binding.v1\0")`. The
coordinator compares this non-secret local commitment in constant time. This
rejects a next envelope signed under a different master key even if a caller
reuses the same public key ID/generation. It proves continuity with the current
carrier's signing secret; it does not prove registry trust or replace protected
key lookup.

The offline publication request and every anchor backend operation now use this
carrier instead of the provisional parsed-field structure. This reconciles the
coordinator to the v1 wire: store UUID is 16 bytes, manifest-key generation is
distinct from anchor-authentication key generation, predecessor presence is
explicit, and writer-authority epoch is authenticated.

An ordinary transition requires two valid carriers, the same store, namespace,
policy, lineage, manifest-key generation, writer-authority epoch, and anchor-key
ID/generation; exact next generation `old + 1`; and a next predecessor digest
equal to the prior selected-manifest digest. Rotation or authority transfer is
an administrative transition and is rejected here.

The protected comparison unit is the whole authenticated envelope. Reads and
final compare-and-swap compare exact size, digest, and every canonical byte.
Parsed-body equality or digest-only equality is insufficient. Attempt registry
binding receives the same owned carriers.

## Bootstrap boundary

Normal publication requires an existing authenticated generation of at least
one. An absent anchor returns `bootstrap_required`; generation-zero and a
synthetic zero predecessor are invalid. A future administrative bootstrap may
perform `absent -> authenticated generation 1` only with null predecessor,
protected authority, create-if-absent semantics, and separate crash review.
L05f does not implement it.

## Limits

The carrier and coordinator remain memory-only and `EXCLUDE_FROM_ALL`. No key
registry, path, I/O, cross-process service, filesystem CAS, persistence, server
hook, provider admission, node, or durability claim is introduced.
