# RB-05 — Logical deletion, CE decision, and media disposition

## Phase A — Logical deletion

1. Authorize immutable target scope and check legal hold/retention/shared references.
2. Create authenticated tombstone and advance serving manifest/index.
3. Verify reads return `MISS_RECOMPUTE`, including through old manifest and restore paths.
4. Remove active object references and queue physical object/replica/snapshot/backup cleanup.
5. Record remaining copies, expiration dates, shared references, and key dependencies.
6. Report **logical deletion** only.

## Phase B — Cryptographic-erasure decision

Proceed only when policy selects CE and the target key scope is acceptable.

1. Identify data-key scope: per-object DEK, group DEK, epoch key, fscrypt tree key, or LUKS volume key.
2. Inventory every usable key copy/path: wrappers, parent derivation, alternate credentials/keyslots, active mapping, kernel/process copies, headers, backups, escrow, snapshots, KMS replicas, exported copies.
3. Drain active files/mappings/processes under RB-08.
4. Execute authorized destroy/revoke operations with quorum where required.
5. Verify key references/authorization are absent and attempt controlled recovery paths that should now fail.
6. Validate the conclusion against the inventory and stated assumptions.
7. Document exact target-data scope and residuals. Do not say physical zeroization.

If an epoch key protects other live objects, per-object CE is not available. Either
logically delete the object, migrate remaining objects and destroy the epoch, or
use an independently wrapped per-object DEK design prospectively.

## Phase C — Physical media

1. Classify media, data sensitivity, reuse/disposition, and provider/device control.
2. Select the current approved clear/purge/destroy technique using NIST SP 800-88r2 and authoritative device/provider instructions.
3. Execute under chain of custody.
4. Verify technique execution; validate that evidence meets organizational requirements.
5. On unsupported/failed/uncertain result, escalate to an alternate technique or destruction.
6. Record final disposition. Discard/TRIM alone is not sanitization evidence.

[CLAIM:PFIR07-C041][CLASS:SOURCE][STATUS:SUPPORTED][SRC:NIST-SP800-88R2 §§2,5,7]

[CLAIM:PFIR07-C042][CLASS:SOURCE][STATUS:SUPPORTED][SRC:NIST-SP800-88R2 §5.4]

[CLAIM:PFIR07-C083][CLASS:UNSUPPORTED][STATUS:REJECTED][SRC:C015,C041-C044]
