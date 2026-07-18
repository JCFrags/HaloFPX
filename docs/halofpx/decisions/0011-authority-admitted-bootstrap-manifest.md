# ADR-0011: authority-admitted bootstrap manifest

- Status: accepted for the disabled offline L05h planner
- Date: 2026-07-18

## Decision

A bootstrap plan must not trust a caller-supplied selected-manifest digest or
object count. The request supplies only a nonzero 256-bit attempt identity and
borrowed exact authenticated-manifest bytes within the existing 1 MiB bound.

The authority snapshot owns a third purpose-separated record for manifest
authentication plus the complete trusted 16-component compatibility
expectation. Manifest, anchor, and bootstrap-admin key tuples and effective
secrets are pairwise distinct. The manifest key generation equals the fixed
manifest-key generation in the authority scope. All key material is copied
into bounded private storage, is exposed by no accessor, and is overwritten at
authority destruction.

Planning performs the existing closed structural parse, computes the
domain-separated digest of the exact complete bytes, builds the exact
generation-one/null-predecessor replay expectation from protected authority
scope, and invokes the accepted manifest verifier with the owned manifest key
and compatibility expectation. Only `authenticated_unadmitted` may proceed.
The selected digest and object count are derived exclusively from that result.
The exact values are then bound into the signed generation-one anchor and the
bootstrap-admin authorization commitment.

Wrong key purpose, key state, key generation, tag, store, namespace, policy,
lineage, generation, predecessor, compatibility component/root, canonical
shape, or length is an administrative rejection with no plan. An all-zero
attempt identity is invalid. Every other 256-bit value is only bound into the
plan; it is not authenticated, checked for historical reuse, or made one-shot
by this stateless planner.

## Limits

The manifest remains semantically unadmitted for restore. L05h proves only that
the offline bootstrap plan is bound to one structurally closed, authenticated,
scope/replay/compatibility-matched manifest. It does not authenticate an
operator, issue or consume an external one-shot token, persist a protected key
registry or replay journal, prove anchor absence, execute create-if-absent,
open objects, restore state, perform I/O, or authorize a cache hit.
