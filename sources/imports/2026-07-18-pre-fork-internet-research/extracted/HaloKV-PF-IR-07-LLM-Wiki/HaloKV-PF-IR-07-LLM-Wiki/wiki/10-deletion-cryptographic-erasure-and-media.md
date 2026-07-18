# Logical deletion, cryptographic erasure, discard, and media sanitization

## Terms must remain distinct

| Term | HaloKV meaning | Evidence allowed | Prohibited wording |
|---|---|---|---|
| Logical deletion | Remove authorization, serving indexes/manifests, object references, and schedule physical object removal/GC | Tombstone, index/manifest generation, GC status, retention dependencies | “Bytes are unrecoverable” |
| Cryptographic erase (CE) | Render all encryption keys needed to decrypt a stated target-data scope inaccessible, under documented assumptions | Key IDs/copies/dependencies, destroy operation, verification/validation, scope | “Physical cells zeroized” |
| Clear/Purge/Destroy media action | Media-specific sanitization categories and techniques under the current sanitization program | Device/provider method, verification, validation, disposition record | Generic “rm/TRIM securely wiped it” |
| Physical destruction | Damage media to the approved destruction threshold for its type and policy | Chain of custody, method, validation, disposition | “Encryption alone destroyed media” |

[CLAIM:PFIR07-C041][CLASS:SOURCE][STATUS:SUPPORTED][SRC:NIST-SP800-88R2 §§2,5,7]

## Logical deletion run

1. Authenticate and authorize the deletion request and scope.
2. Create a signed/MACed tombstone with tenant/project/prefix/namespace, source object/manifest generation, time, requester, retention/legal-hold result, and deletion ID.
3. Advance the serving manifest/index so the object is unreachable before storage GC.
4. Revoke future key authorization where the deletion scope justifies it.
5. Remove object files from active paths and queue replicas, snapshots, backups, and quarantine copies according to policy.
6. Record shared-object references: a shared ciphertext cannot be physically removed while an authorized live reference remains.
7. Confirm future reads return miss and do not resurrect from older manifests/backups.
8. Report this as logical deletion unless CE or media sanitization is separately completed.

[CLAIM:PFIR07-C067][CLASS:SYNTHESIS][STATUS:REQUIRED][SRC:C041-C044]

## Cryptographic-erasure scope

A shared epoch key normally makes CE **epoch-scoped**, not object-scoped. Destroying it also removes every object under that key, while retaining it means one deleted object remains decryptable wherever ciphertext and authorization/key access survive. Fine-grained per-object CE requires a random per-object DEK (or independently erasable small group DEK) wrapped by a parent key, plus deletion of every usable DEK/wrapper copy and backup dependency.

CE preconditions include:

* target data was encrypted with sufficient, correctly implemented cryptography;
* the relevant key scope is precisely identified;
* all key copies, wrappers, alternate credentials, parent derivation paths, escrow, KMS replicas, LUKS headers/keyslots, snapshots, backups, and process-held copies are inventoried;
* the destroy operation makes every usable target-data key inaccessible;
* active files/mappings/processes are drained as required;
* verification and validation are performed and documented;
* residual memory, future cryptanalysis, implementation, and media assumptions are stated.

[CLAIM:PFIR07-C042][CLASS:SOURCE][STATUS:SUPPORTED][SRC:NIST-SP800-88R2 §5.4]

[CLAIM:PFIR07-C068][CLASS:SYNTHESIS][STATUS:CONDITIONAL][SRC:C042-C044,C054]

CE may be the selected purge technique under a media policy when its requirements are met. This bundle does not assert that any specific HaloKV deployment, KMS, drive, filesystem, or provider meets those requirements without deployment evidence.

## LUKS keyslot deletion is not automatically CE

A keyslot or passphrase removal can leave alternate keyslots, an active mapping, a volume key in memory/keyring, detached headers, header backups, escrow, and snapshots. It also does not overwrite payload data. Credential/keyslot rotation and payload-key reencryption are distinct operations.

[CLAIM:PFIR07-C021][CLASS:SOURCE][STATUS:SUPPORTED][SRC:CRYPTSETUP-2.8.6 cryptsetup-erase and luksKillSlot]

## Discard/TRIM

Passing discard through dm-crypt can reveal which regions are unused and may expose filesystem/allocation patterns. A discard acknowledgement describes a storage command path, not a verified conclusion about all physical cells, remapped blocks, over-provisioned flash, replicas, or provider media.

Default: `discard` off for the encrypted cache volume. A performance exception records leakage acceptance and still uses the normal deletion/sanitization process.

[CLAIM:PFIR07-C015][CLASS:SOURCE][STATUS:SUPPORTED][SRC:LINUX-DMCRYPT-7.2RC3; CRYPTSETUP-2.8.6 common options]

[CLAIM:PFIR07-C069][CLASS:SYNTHESIS][STATUS:RECOMMENDED][SRC:C015,C041-C044]

[CLAIM:PFIR07-C083][CLASS:UNSUPPORTED][STATUS:REJECTED][SRC:C015,C041-C044]

## Physical-media sanitization

Media retirement/reuse uses the organization's current NIST SP 800-88r2-aligned program and current authoritative device/provider instructions. Select a technique based on media type, sensitivity, reuse/disposition, device capabilities, and required assurance. The process includes:

1. media and data classification;
2. chosen clear/purge/destroy technique and rationale;
3. authorized operator and chain of custody;
4. command/tool/device capability and version;
5. verification that the technique executed as expected;
6. validation that the result satisfies the organization's requirement;
7. exception/failure handling, including destruction where appropriate;
8. disposition and retained certificate/report.

Do not invent a universal number of overwrite passes, SSD secure-erase guarantee, cloud physical-sanitization guarantee, or cryptographic-module compliance result. Use current media/vendor/provider evidence and record uncertainty.

[CLAIM:PFIR07-C044][CLASS:SOURCE][STATUS:SUPPORTED][SRC:NIST-SP800-88R2 §§6-7]

## Deletion report language

Acceptable:

> “HaloKV logical deletion completed for tenant T/project P at manifest generation G. Serving references and authorization were removed; active replicas are queued for GC. Backup B remains until date D under retention policy. No physical-sanitization claim is made.”

> “Cryptographic erase was validated for epoch E under key inventory I: all listed wrapped-key, escrow, and backup dependencies were rendered inaccessible. The conclusion applies to the stated target-data scope and assumptions; it is not a claim of physical zeroization.”

Unacceptable: “The file was securely wiped because it was unlinked,” “TRIM destroyed the data,” “removing one LUKS slot erased the disk,” or “encryption guarantees regulatory compliance.”
