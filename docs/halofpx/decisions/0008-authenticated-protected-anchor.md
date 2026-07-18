# ADR-0008: authenticated protected-anchor wire contract

- Status: accepted for the disabled offline L05e codec
- Date: 2026-07-18

## Decision

The protected publication anchor is one closed deterministic-CBOR authenticated
envelope. Its exact schema is `authenticated-publication-anchor-v1` in the v1
CDDL. It distinguishes the anchor-authentication key generation from the
selected manifest-key generation and binds the writer-authority epoch. Attempt
identifiers are not anchor identity and confer no authority.

Generation one requires a null predecessor. Later generations require a digest.
Normal publication advances exactly one generation and names the prior selected
manifest digest. The complete canonical authenticated envelope is the future
protected compare-and-swap unit; parsed subsets are not sufficient CAS identity.

The cryptographic domains are:

```text
K_anchor = HMAC-SHA-256(K_master,
  "halofpx.anchor-key.v1\0" || key_id_len:u16be || key_id ||
  store_uuid || namespace_id || anchor_key_generation:u64be)
anchor_tag = HMAC-SHA-256(K_anchor,
  "halofpx.anchor-auth.v1\0" || exact DCBOR(anchor-auth-input-v1))
anchor_digest = SHA-256(
  "halofpx.anchor.v1\0" || exact DCBOR(authenticated anchor))
```

Verification first performs bounded structural parsing, then exact protected-key
lookup and authentication, then exact trusted scope/authority comparison, and
finally exact replay comparison when a trusted expected anchor is supplied.
Key lookup never scans or falls back. Unknown, revoked, read-disabled, stale,
forked, skipped, differently scoped, or unauthenticated state is a miss.
Anchor authentication key IDs use the project-wide registered-ID boundary of
1--128 ASCII bytes without NUL; there is no normalization or case folding.

Authentication does not itself provide rollback resistance. The selected
generation is the lineage high-water only while this anchor and its key/authority
state remain protected outside the cache root. Compromise or rollback of both is
outside ADR-0004's claim.

## L05e boundary

The codec is memory-only, bounded to 1 KiB, excluded from default builds, and
returns only `authenticated_unadmitted`. It performs no path access, filesystem
write, provider call, server integration, state restore, or hit admission.

Concrete persistence remains blocked on a protected key registry, first-store
administrative bootstrap, a cross-process exact-envelope CAS substrate, and
filesystem durability qualification.
