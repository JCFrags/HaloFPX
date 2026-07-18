# L05i authenticated bootstrap-admin token

Status: accepted offline after independent adversarial review.

L05i replaces the caller-supplied bootstrap attempt identity with a separate,
target-owned, authenticated administrative envelope. The closed 2 KiB maximum
deterministic-CBOR token binds the complete public bootstrap scope, exact
manifest identity, generation-one/null-predecessor transition, purpose-
separated key tuples, protected-registry snapshot and policy commitments, the
next expected authorization sequence, and a 256-bit command identity.

The offline authority first verifies the token under its owned bootstrap-admin
key, then separately authenticates the exact manifest and requires the two
manifest digests to agree. Only authenticated token fields can enter the anchor
or opaque `authorized_unexecuted` plan. The plan carries the exact token digest,
command identity, sequence, and a private plan commitment; it carries no key or
token bytes.

The first local review found that the draft codec disagreed with the accepted
CDDL by encoding registered IDs as byte strings and representing the null
predecessor with a boolean plus zero digest. The wire was corrected to text
strings and one CBOR `null`. An independent Python encoder now fixes the exact
399-byte envelope, authentication tag, and envelope digest. The C++ test checks
that independent digest and rejects every truncation point and every single-
byte mutation. Independent review then found and closed an outer-envelope tag-
coverage mismatch and a noncanonical-fixture count error before final ACCEPT.

This milestone is stateless. The immutable authority compares the token
sequence with `last_consumed_sequence + 1`, but does not advance that high-
water value. Reusing the same valid token against the same snapshot therefore
continues to return `authenticated_unconsumed` and an unexecuted plan. A later
protected atomic compare-and-advance operation is required before any one-shot
or replay-prevention claim.

The new token and authority targets remain `STATIC EXCLUDE_FROM_ALL`. They have
no path, filesystem, network, server/provider, storage, or node dependency.
The clean Windows CPU/WebUI-off Release control passed all 21 HaloFPX CTests
and seven focused inherited tests. Token, authority, and independent golden
checks each passed 200 separate processes. Exact hashes and exclusions are in
`evidence/l05i-bootstrap-token-repeat-receipt.json`.

Still closed: credential-provider and principal identity, token issuance and
revocation, protected registry/high-water persistence and rollback protection,
atomic consumption, conclusive protected-anchor absence, create-if-absent,
filesystem durability, cache admission, server integration, and node use.
