# Quarantine and incident response

## Detection classes

| Class | Examples | Immediate serving behavior |
|---|---|---|
| Object-authentication failure | Tag failure, AAD mismatch, wrong key, CID mismatch, malformed envelope | `MISS_RECOMPUTE`; remove from serving index; quarantine only if evidence policy requires |
| Freshness failure | Old generation, revoked epoch, superseded manifest, tombstoned object | `MISS_RECOMPUTE`; preserve metadata for rollback investigation |
| Nonce/key incident | Suspected reuse, counter rollback, exposed rank key, unauthorized unwrap | Stop affected epoch reads/writes; new epoch; quarantine affected ciphertext |
| Lower-layer integrity failure | dm-integrity I/O error, filesystem corruption, LUKS header anomaly | Stop affected volume; do not “repair” object authentication; image/evidence as authorized |
| Principal/policy incident | Wrong tenant mapping, over-broad dedup scope, unauthorized public promotion | Stop affected scopes; revoke policy/key access; audit every touched manifest/object |
| Publisher compromise | Validly authenticated but malicious public/system content | Revoke publication generation/key; advance rollback floor; remove system prefix from serving |
| Deletion/backup incident | Deleted content restored, unexpected snapshot/header backup, failed destroy | Isolate copy; preserve chain of custody; update key-copy and retention inventory |

## Quarantine properties

Quarantine is a separate non-serving security domain:

* separate path, storage account, credentials, and encryption key;
* no lookup path, symlink, bind mount, object-store alias, or manifest pointer reachable by normal cache service;
* ciphertext preserved by default; plaintext extraction only in an authorized analysis environment;
* immutable intake record with source path, hashes, detection reason, time, rank, scope, epoch, and collector;
* retention deadline or incident/legal-hold authority;
* access logging and least privilege;
* disposition action: release only through revalidation/re-encryption, delete logically, CE under stated scope, or media process.

Quarantine is not a place to bypass failed authentication. An analyst may inspect raw bytes, but the serving system never consumes unauthenticated content from it.

## Incident sequence

1. **Contain:** stop admission and serving for the affected scope/epoch; preserve the authoritative recomputation path if trustworthy.
2. **Classify:** key compromise, nonce uncertainty, object corruption, replay, lower-layer failure, authority error, publisher compromise, or deletion failure.
3. **Preserve:** collect ciphertext, manifests, key IDs—not plaintext keys—logs, KMS audit, allocator state, host/mapping status, and hashes under chain of custody.
4. **Revoke:** disable unwrap/derive authorization; mark epoch/key `COMPROMISED`; revoke principal/publisher credentials as applicable.
5. **Drain:** terminate workers, close files/mappings, remove fscrypt/kernel keys where configured, and record any incomplete busy state.
6. **Re-key:** establish a new rank/namespace epoch with fresh key and nonce allocator identity. Never resume uncertain state.
7. **Invalidate:** advance trusted epoch/generation floors; publish tombstone/revocation records; remove affected entries from serving indexes.
8. **Recover:** recompute from trusted source or authenticate and re-encrypt only through quarantine import into the new context.
9. **Assess copies:** backups, snapshots, LUKS headers, detached headers, KMS replicas, escrow, exports, crash dumps, swap, and peer copies.
10. **Communicate:** notify accountable tenant/key/publisher owners according to incident policy; do not overstate erasure.
11. **Close:** document cause, affected scope, dwell window, key/nonce usage, residual risk, and corrective controls; update test vectors/runbooks.

[CLAIM:PFIR07-C070][CLASS:SYNTHESIS][STATUS:REQUIRED][SRC:C031-C033,C040,C058-C065]

## Failure-to-miss mapping

Externally, every invalid object maps to the same result. Internally:

| Internal reason | Security action | Serving result |
|---|---|---|
| `KEY_NOT_AUTHORIZED` | Audit denied key request; no broad key search | `MISS_RECOMPUTE` |
| `KEY_REVOKED` | Enforce epoch floor; investigate stale index | `MISS_RECOMPUTE` |
| `FORMAT_BOUNDS` | Quarantine/delete malformed object; increment abuse metric | `MISS_RECOMPUTE` |
| `AAD_EXPECTED_DIFFERENT` | Investigate path/rank/tenant substitution | `MISS_RECOMPUTE` |
| `AEAD_TAG_FAIL` | Quarantine sample/evidence; invalidate candidate | `MISS_RECOMPUTE` |
| `CID_FAIL` | Treat as semantic corruption/bug; invalidate candidate | `MISS_RECOMPUTE` |
| `REPLAY_FLOOR` | Preserve old generation metadata; enforce tombstone | `MISS_RECOMPUTE` |
| `OBJECT_MISSING` | Remove stale manifest entry or rebuild generation | `MISS_RECOMPUTE` |
| `UNSUPPORTED_CRITICAL_EXTENSION` | Format-policy miss; no best-effort parsing | `MISS_RECOMPUTE` |

Reason detail is not returned to the tenant-facing caller. Aggregate observability must avoid key IDs, CIDs, plaintext, nonces paired with keys, and tenant-crossing timing oracles.

## Public/system incident

A compromised publisher can create cryptographically valid harmful content. Response requires publication-key/credential revocation, generation rollback floor, manifest withdrawal, new publisher epoch/key, review of source provenance, and tenant notification according to impact. Object AEAD or a signature does not prove semantic safety.

## Availability during incident

Fail-closed behavior may create a recomputation surge. Use per-scope admission control, duplicate-work coalescing only within the same authorized scope, backoff, quotas, and circuit breakers. Never relax tag/AAD/freshness checks to preserve hit rate.

[CLAIM:PFIR07-C063][CLASS:SYNTHESIS][STATUS:REQUIRED][SRC:C030,C034,C058-C059]
