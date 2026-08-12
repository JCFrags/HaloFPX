# RB-02 — Epoch rotation and revocation

## Trigger classes

* Scheduled rotation or policy change.
* Rank replacement, clone prevention, or namespace migration.
* Principal membership/sharing-class change.
* Suspected key disclosure, unauthorized unwrap, nonce reuse, or allocator uncertainty.
* Public/system publisher key rollover.

## Routine rotation

1. Approve scope and identify authority/tenant/project/prefix/namespace/rank.
2. Inventory old epoch state, manifest generations, active workers, backups, and rollback window.
3. Generate a fresh epoch root and derive distinct `K_enc`, `K_id`, and `K_manifest` according to policy. Do not reuse nonce state.
4. Activate the new epoch for writes; old epoch becomes read-only for the bounded migration window.
5. Publish a new authenticated manifest generation naming the active epoch and old-read policy.
6. On old-epoch hit, authenticate fully, then recompute/re-encrypt into new epoch with fresh nonce. Any failure is miss.
7. End old-read window; advance epoch floor; remove old entries from serving index.
8. Stop old-key leases, drain workers, and verify key/mapping/filesystem state as applicable.
9. Mark old epoch deactivated. Destroy only after backup/escrow/legal-hold decisions permit and evidence is complete.

## Emergency revocation

1. Mark old epoch `COMPROMISED`; stop reads and writes immediately for affected scope.
2. Disable broker authorization and revoke principal/publisher credentials as applicable.
3. Stop admission; drain/terminate workers; close files/mappings according to RB-08.
4. Create fresh epoch and allocator identity.
5. Advance trusted floors and publish revocation/tombstone metadata from a trusted authority.
6. Quarantine affected ciphertext and manifests; do not serve old epoch during migration.
7. Recompute from trusted source into the new epoch.
8. Inventory exports, peers, backups, headers, snapshots, KMS replicas, dumps, and escrow.
9. Document residual memory/media exposure and destruction status.

## Completion criteria

Rotation/revocation is not complete until serving policy, broker authorization,
active processes, lower-layer key state, backup dependencies, and manifest floors
are all accounted for. A keyslot/passphrase change alone does not establish
payload-key or HaloKV object-key rotation.

[CLAIM:PFIR07-C022][CLASS:SOURCE][STATUS:SUPPORTED][SRC:CRYPTSETUP-2.8.6 luksChangeKey and reencrypt]

[CLAIM:PFIR07-C070][CLASS:SYNTHESIS][STATUS:REQUIRED][SRC:C031-C033,C040,C058-C065]
