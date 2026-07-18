# L05j authenticated protected-registry snapshot

Status: accepted offline after independent adversarial review.

L05j removes the caller-trusted registry ID, epoch, snapshot digest, policy
commitment, and last-consumed sequence from the bootstrap-authority
configuration. The authority now accepts only borrowed exact snapshot bytes
and a fourth purpose-separated authentication key. It synchronously verifies
the bounded snapshot, derives all registry comparisons and `H + 1` from the
authenticated body, and retains neither borrowed bytes nor the registry key.

The target-owned 1 KiB maximum deterministic-CBOR format is a two-field outer
envelope containing the complete four-field authentication-input map and its
HMAC-SHA-256 tag. The body binds format, kind, registry ID and epoch, a public
authority-base-scope commitment, an opaque policy commitment, and the high-
water sequence. An independent Python encoder fixes the exact 156-byte
envelope, tag, envelope digest, and secret-derived private authority binding.

The public base scope binds all four key tuples and all immutable bootstrap and
compatibility inputs. The public v2 authority scope adds the authenticated
registry identity, exact envelope digest, policy commitment, and high-water.
The private authority snapshot additionally binds a transient secret-derived
registry value. All four key tuples and HMAC-effective secrets must be pairwise
distinct.

Independent review found and closed three material defects before promotion.
Public scope helpers first allowed an oversized registered-ID size to drive an
out-of-bounds read. The bounded encoder now validates IDs before copying. The
registry parser also compared signed Windows `char` values directly, allowing
non-ASCII bytes to evade its ASCII check; it now checks unsigned byte values,
with independently re-authenticated `0x80` and `0xff` hostile fixtures. Review
also required the final sixth key-tuple collision and explicit opaque-carrier
ownership/private-binding tests. Final re-review returned ACCEPT.

This milestone proves snapshot integrity and exact base-scope membership only.
An older correctly authenticated snapshot remains accepted, and the same token
can still plan repeatedly against the same snapshot. L05j therefore makes no
freshness, anti-rollback, durable high-water, atomic consumption, or one-shot
claim. The opaque policy commitment is authenticated but not evaluated.

The new codec and authority remain `STATIC EXCLUDE_FROM_ALL`, with no path,
filesystem, network, provider, server, persistence, or node dependency. The
clean Windows CPU/WebUI-off Release control passed 24/24 HaloFPX CTests, seven
focused inherited tests, and 600/600 separate registry, authority, and golden
processes. Exact hashes are in
`evidence/l05j-protected-registry-repeat-receipt.json`.

Still closed: credential custody and principal identity, registry freshness and
rollback resistance, durable compare-and-advance, command consumption,
conclusive protected-anchor absence, create-if-absent, filesystem durability,
cache admission, server integration, and node use.
