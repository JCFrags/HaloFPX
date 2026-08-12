# Backup, restore, snapshots, and export

## Default: do not back up regenerable cache content

Persistent cache backup increases secret-copy inventory, rollback risk, deletion propagation, and restore complexity. The default is to back up only configuration and authority records needed to rebuild—not rank-local cache objects. Business-approved exceptions must document why recomputation cannot meet recovery objectives.

[CLAIM:PFIR07-C064][CLASS:SYNTHESIS][STATUS:RECOMMENDED][SRC:C013-C014,C040-C044]

## Coherent backup set

A usable and auditable backup may require all of the following as a coordinated recovery point:

* encrypted object files;
* authenticated manifests and serving-index generation;
* format/schema/codec/chunking versions;
* authority, tenant, project, prefix, namespace, sharing, rank, and epoch metadata;
* wrapped child roots or KMS references and wrapping-key versions;
* nonce allocator state **only for forensic continuity**—restored writers still receive a new epoch;
* trusted epoch/generation floors and deletion tombstones;
* LUKS2 header/detached-header backup if raw-volume recovery requires it;
* fscrypt policy/key-descriptor metadata if tree-level recovery requires it;
* backup encryption metadata, retention, legal hold, and restore authorization.

Do not store plaintext object keys in the backup. Backup encryption uses a separately governed backup KEK; simply nesting cache ciphertext does not eliminate access-policy and rollback obligations.

## LUKS header backups

A LUKS header backup is key-bearing recovery material. It can restore keyslots/metadata and may reactivate access that operators believed revoked. Store it separately, encrypt and access-control it, inventory copies, bind it to the volume UUID/recovery point, and include it in rotation, incident, and CE analysis.

[CLAIM:PFIR07-C014][CLASS:SOURCE][STATUS:SUPPORTED][SRC:CRYPTSETUP-2.8.6 cryptsetup-luksHeaderBackup]

[CLAIM:PFIR07-C082][CLASS:UNSUPPORTED][STATUS:REJECTED][SRC:C014,C021-C022,C042]

## Snapshot rules

* A live filesystem/block snapshot must use an application-aware quiesce or an explicitly crash-consistent recovery design.
* A snapshot of nonce allocator state must never resume writes under the same restored AEAD key; allocate a new epoch.
* A snapshot can preserve deleted objects, old manifests, old LUKS headers, and wrapped keys. It participates in retention and erasure scope.
* Cross-region/provider replicas are separate copies in the key and deletion inventory.
* Snapshot names and tags are not trusted freshness state unless protected by the authority.

## Restore workflow

All restore targets start in a **non-serving quarantine namespace** with separate credentials and no symlink/path into the live cache.

1. Authorize the restore and record source, owner, reason, scope, and expected revision.
2. Verify backup transport/container hashes and signatures where available.
3. Restore ciphertext and metadata without unlocking object keys.
4. Load current authority policy, revocation state, epoch floors, tombstones, and accepted format versions.
5. Validate lower-layer volume/header identity before activation; never apply a header backup to an unverified target.
6. Obtain only the old keys explicitly authorized for validation; do not reactivate compromised/destroyed epochs for serving.
7. Authenticate every manifest and enforce current freshness/deletion policy.
8. Validate objects eagerly for high assurance or lazily with a declared sampling/coverage policy; any failure is miss/quarantine.
9. Re-encrypt or recompute accepted data into a **new destination rank and epoch**, with fresh nonces and new manifests.
10. Atomically publish only the new destination manifest/index.
11. Remove temporary keys, close mappings, and apply quarantine retention/disposition policy.
12. Produce a restore report with accepted/rejected counts and unresolved residual risks.

[CLAIM:PFIR07-C065][CLASS:SYNTHESIS][STATUS:REQUIRED][SRC:C013-C014,C058-C060,C064]

## Restore of an old deleted epoch

A valid old tag does not override a current tombstone or epoch floor. The data may be retained as incident/legal evidence under separate authority, but it does not return to cache service. If policy authorizes reintroduction, it is treated as a new authoritative import and receives a new scope, epoch, key, nonce, CID policy, and manifest.

## Export

Export is an authorization and cryptographic-context transition:

1. Identify source and destination authorities, tenants/projects, sharing class, purpose, and recipient.
2. Authorize the principal for source decrypt and destination write; log both decisions.
3. Authenticate source manifest/object and enforce freshness/tombstones.
4. Decrypt into bounded memory; never export from a tag-failed or stale cache entry.
5. Transform only through an approved semantic export codec.
6. Recompute destination-scoped CID; do not reuse a source-private CID as a global identifier.
7. Encrypt under destination key/AAD with a fresh nonce and publish a destination manifest.
8. Erase temporary plaintext/key buffers to the practical software limit and report residuals.

Copying ciphertext is acceptable only as an opaque backup/evidence transfer; it does not transfer principal authorization or make the object directly consumable by the destination.

[CLAIM:PFIR07-C066][CLASS:SYNTHESIS][STATUS:REQUIRED][SRC:C030,C034,C052-C057]

## Backup/restore test cadence

At least one controlled restore drill should exercise: missing key, wrong key, revoked epoch, corrupted tag, modified AAD, stale manifest, absent object, duplicate object, old tombstoned object, wrong LUKS header, busy mapping, and restored nonce state. The expected serving result for each invalid cache object is `MISS_RECOMPUTE`.
