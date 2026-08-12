# RB-04 — Backup exception and quarantine restore

## Backup decision gate

The default answer is no cache-content backup. An exception requires an approved
RPO/RTO, retention, legal-hold, key ownership, restore authority, and deletion
impact analysis.

## Backup procedure

1. Identify a coherent manifest generation and quiesce/publish boundary.
2. Capture encrypted objects, authenticated manifests, format/codec versions, wrapped key references, epoch floors, and tombstones.
3. Capture LUKS header/detached-header material only if required, and classify it as key-bearing sensitive material.
4. Store backup under a separately governed backup encryption key and access policy.
5. Record all replicas, KMS/escrow dependencies, retention expiry, and deletion propagation.
6. Hash/sign the backup container where supported and retain verification evidence.

## Restore procedure

1. Create a non-serving quarantine target with separate credentials/keys.
2. Authorize restore; record source, reason, expected revision, and owner.
3. Verify backup hashes/signatures and target lower-layer identity.
4. Restore ciphertext without exposing it to the live lookup path.
5. Load current authority policy, epoch floors, tombstones, and accepted format/suites.
6. Authenticate manifests; reject stale, revoked, unknown, or unsupported generations.
7. Validate objects according to the approved coverage policy. Every invalid entry is miss/quarantine.
8. Allocate a new destination rank and epoch; restored allocator state is not reused.
9. Re-encrypt or recompute accepted content into destination scope with new CIDs/nonces/manifests.
10. Atomically publish the new destination generation only after acceptance.
11. Remove temporary key access, close mappings, and apply quarantine retention/disposition.
12. Issue restore report with counts, failures, retained dependencies, and residual risks.

## Prohibitions

* No direct restore into a live serving directory or object-store prefix.
* No “try every keyslot/tenant key” salvage path.
* No revival of a tombstoned object solely because its old tag verifies.
* No reuse of restored nonce counter under the restored key.
* No CE assertion while backup/header/escrow dependencies remain.

[CLAIM:PFIR07-C064][CLASS:SYNTHESIS][STATUS:RECOMMENDED][SRC:C013-C014,C040-C044]

[CLAIM:PFIR07-C065][CLASS:SYNTHESIS][STATUS:REQUIRED][SRC:C013-C014,C058-C060,C064]
