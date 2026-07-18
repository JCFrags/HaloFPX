# ADR-0012: authenticated bootstrap-admin token

- Status: accepted for the disabled offline L05i token verifier
- Date: 2026-07-18

## Decision

First-store bootstrap planning requires a separately authenticated,
target-owned bootstrap-admin token. The token is a closed, bounded,
deterministic-CBOR envelope authenticated with a purpose-separated
HMAC-SHA-256 key. Its only positive verifier state is
`authenticated_unconsumed`.

The frozen v1 key-derivation, authentication, and envelope-digest domains are
`halofpx.bootstrap-token-key.v1`, `halofpx.bootstrap-token-auth.v1`, and
`halofpx.bootstrap-token.v1`, each followed by exactly one NUL byte. Registered
IDs are deterministic-CBOR text strings. The generation-one predecessor is one
CBOR `null`, not a boolean or a zero digest.

The authenticated body fixes format major/minor, operation
`first-store-bootstrap`, exact store UUID, namespace, policy epoch,
checkpoint lineage, manifest-authentication key ID/generation, compatibility
root, writer-authority epoch, anchor-authentication key ID/generation, target
generation `1`, null predecessor, selected authenticated-manifest digest,
public authority-scope commitment, protected-registry ID/epoch/snapshot digest
and policy digest, nonzero authorization sequence, and a nonzero 256-bit
command ID. The authentication wrapper fixes the bootstrap-admin key ID,
generation, algorithm, and tag. Unknown, duplicate, reordered, noncanonical,
indefinite, missing, or extra data is rejected.

The public authority-scope commitment is a digest of explicit non-secret
scope, key tuples, compatibility identity, and protected-registry view. It is
distinct from the authority's private secret-derived snapshot commitment. A
token issuer never needs anchor, manifest, or authority-snapshot secret
material.

The authority request contains only borrowed exact token bytes and borrowed
exact manifest bytes. It derives command identity and authorization sequence
only from an authenticated token, separately authenticates the exact manifest,
and requires the two selected-manifest digests to match. A successful plan
binds the exact token-envelope digest into its private plan commitment and
remains only `authorized_unexecuted`.

## Sequence and replay boundary

The immutable authority snapshot contains a protected-registry high-water
view named `last_consumed_sequence`. Zero is valid for a new registry;
`UINT64_MAX` invalidates the authority. A token is compatible only when its
authorization sequence is exactly `last_consumed_sequence + 1` and all
registry identities and commitments match.

This is comparison against a trusted snapshot, not consumption. Re-verifying
or replanning the same token against the same snapshot succeeds. L05i has no
historical command registry, compare-and-advance primitive, cross-process
coordination, restart freshness proof, rollback protection, or one-shot claim.
Only a later protected atomic transition from high-water `H` to `H + 1`, bound
to the registry snapshot and command ID, may claim consumption.

## Failure and execution boundary

Structural, key-state, key-generation, authentication, scope, authority,
registry, sequence, or manifest mismatch produces no authenticated token
carrier and no plan. Planner-level token failure is
`authorization_rejected`. No unauthenticated token field may influence a
manifest policy, anchor body, or plan.

The token proves authorization by possession of the configured admin key. It
does not prove a human operator identity, credential-provider decision, or
principal authentication. L05i adds no issuance service, secret store,
protected registry storage, token consumption, conclusive anchor-absence
check, create-if-absent, filesystem operation, server/provider linkage,
persistent write, cache admission, restore, hit, or node behavior.
