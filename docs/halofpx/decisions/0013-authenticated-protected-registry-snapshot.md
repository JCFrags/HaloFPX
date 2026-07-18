# ADR-0013: authenticated protected-registry snapshot

- Status: accepted for the disabled offline L05j verifier
- Date: 2026-07-18

## Decision

The bootstrap authority no longer accepts caller-trusted registry ID, epoch,
snapshot digest, policy digest, or last-consumed sequence scalars. L05j accepts
only borrowed exact registry-snapshot bytes plus a fourth purpose-separated
registry-authentication key. Construction synchronously verifies the snapshot,
copies only authenticated non-secret values and exact envelope identity, and
retains no borrowed bytes.

The closed deterministic-CBOR snapshot is at most 1 KiB. Its body fixes format
major/minor, kind `bootstrap-authorization-registry`, registered registry ID,
nonzero registry epoch, nonzero public authority-base-scope commitment,
nonzero opaque registry-policy commitment, and last-consumed authorization
sequence. The nested authentication input additionally fixes the registry-
authentication key ID, HMAC-SHA-256 algorithm, and key generation. HMAC covers
that complete exact map; the two-field outer envelope adds only the tag.

The frozen domains, each followed by exactly one NUL byte, are:

- `halofpx.bootstrap-authority-base-scope.v1`
- `halofpx.registry-snapshot-key.v1`
- `halofpx.registry-snapshot-auth.v1`
- `halofpx.registry-snapshot.v1`
- `halofpx.bootstrap-authority-scope.v2`

The public base-scope commitment binds the public tuples for bootstrap-admin,
anchor, manifest, and registry-authentication keys; store UUID; namespace;
policy epoch; checkpoint lineage; manifest-key generation; writer-authority
epoch; all compatibility components; and compatibility root. It excludes
registry dynamic state and all secrets. The complete public authority-scope v2
commitment binds that base scope plus authenticated registry ID, epoch, exact
snapshot-envelope digest, policy commitment, and last-consumed sequence.

The four key purposes require active disposition, valid nonzero generation,
bounded nonempty secret material, and pairwise-distinct effective tuple and
secret. A valid registry snapshot must authenticate under the configured
registry key and match the computed base scope. The exact authenticated
envelope digest becomes the token's registry snapshot identity. Sequence
`UINT64_MAX` rejects construction; otherwise the expected token sequence is
the authenticated high-water plus one.

## Integrity and rollback boundary

L05j proves only integrity and exact base-scope binding under the configured
memory key. It does not prove that the snapshot or key came from protected
storage, that the epoch is latest, or that the high-water cannot roll back.
Reconstructing an authority from an older correctly authenticated snapshot is
deliberately still accepted and must remain a negative-capability test.

The opaque policy commitment is authenticated but not semantically evaluated.
Registry-key possession authenticates the declaration; it does not establish a
human operator, credential-provider decision, or current revocation state.

## Execution boundary

L05j adds no registry path, credential provider, persistent high-water,
compare-and-advance, command consumption, one-shot execution, cross-process
coordination, conclusive protected-anchor absence, create-if-absent,
filesystem, server/provider linkage, persistent write, cache admission,
restore, hit, or node behavior. A successful bootstrap plan remains only
`authorized_unexecuted`.
