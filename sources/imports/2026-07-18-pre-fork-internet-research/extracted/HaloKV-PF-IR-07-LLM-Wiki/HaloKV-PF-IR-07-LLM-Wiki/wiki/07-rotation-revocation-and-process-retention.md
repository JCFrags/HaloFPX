# Rotation, revocation, peer separation, and process retention

## Rotation taxonomy

| Operation | Changes | Does not necessarily change |
|---|---|---|
| LUKS passphrase/keyslot change | Credential/KDF wrapper for a volume key | Payload volume key or ciphertext |
| LUKS payload reencryption | Volume key and/or cipher parameters according to operation | HaloKV object keys, tenant AAD, manifests |
| fscrypt new policy/tree | Directory policy and master-key domain for new/migrated files | Object semantic key unless explicitly coordinated |
| HaloKV wrapping-key rotation | Parent KEK version or wrapped child record | Object ciphertext if child data key is unchanged |
| HaloKV epoch rotation | Rank/namespace write key and nonce domain | Old ciphertext; old epoch may remain read-only temporarily |
| HaloKV data-key rotation | Object ciphertext under new key/DEK | Scoped plaintext semantics/CID if policy leaves `K_id` unchanged |
| Identifier-key rotation | Private CID/index values | Payload ciphertext unless migrated separately |

[CLAIM:PFIR07-C022][CLASS:SOURCE][STATUS:SUPPORTED][SRC:CRYPTSETUP-2.8.6 luksChangeKey and reencrypt]

## Routine epoch rotation

Preferred cache operation:

1. Create a fresh random epoch root under the same approved authority/tenant/project/prefix/namespace/rank context.
2. Derive new purpose keys and allocate a new nonce state.
3. Mark the old epoch `READ_ONLY`; all new writes use the new epoch.
4. Publish a manifest generation that identifies the new active epoch and accepted old-read window.
5. On a hit from the old epoch, authenticate under the old policy, then optionally recompute/re-encrypt into the new epoch.
6. Expire the old-read window, invalidate remaining old manifest entries, drain keys from workers, and mark the old epoch `DEACTIVATED`.
7. Destroy old key material only after backup, escrow, legal-hold, and rollback decisions permit; record scope and evidence.

This approach avoids risky bulk in-place cache transformation. A cache entry that cannot be migrated safely is discarded and recomputed.

## Compromise rotation

For suspected key compromise or nonce reuse, skip the read-only serving period unless the incident commander explicitly accepts it. Mark the affected epoch `COMPROMISED`, stop writes and serving reads, establish a new epoch, invalidate manifests, quarantine old ciphertext as evidence, and recompute from trusted inputs.

[CLAIM:PFIR07-C070][CLASS:SYNTHESIS][STATUS:REQUIRED][SRC:C031-C033,C040,C058-C065]

## Rank and peer separation

Each rank receives only its own rank-epoch key branch. Objects bind `rank_id` in AAD. Peer transfer has three safe forms:

* **No transfer:** destination recomputes under its rank key.
* **Authenticated import:** source object is opened under source authorization in a controlled transfer service, then re-encrypted under destination rank/epoch AAD.
* **Shared group namespace:** both ranks are members of an explicitly authorized sharing class with a group-owned key domain and a reviewed nonce allocation. This is not the default.

Copying a ciphertext file and editing its path, manifest, or metadata cannot make it valid for another rank. Sharing an AEAD key across peers without a rigorous nonce partition is prohibited.

## Busy-file and mapping behavior

### fscrypt

Removing a master key does not evict per-file keys for open or otherwise active files. Revocation procedure must stop new opens, close descriptors, release memory maps and working-directory/root references, terminate stubborn processes, retry key removal, and inspect the kernel's removal status. Restarting a worker or host may be the only practical way to bound residual process/kernel state, but it is still not a physical memory-erasure guarantee.

[CLAIM:PFIR07-C005][CLASS:SOURCE][STATUS:SUPPORTED][SRC:LINUX-FSCRYPT-7.2RC3 §Removing keys]

### dm-crypt/LUKS2

A mapping close requires no active users. Deferred close waits for the final user and therefore is not completion evidence. Stop I/O, unmount, inspect open handles, close the mapping, verify device-mapper state, and remove any linked volume key. LUKS keyslot removal while the mapping remains active does not remove the in-kernel active volume key.

[CLAIM:PFIR07-C019][CLASS:SOURCE][STATUS:SUPPORTED][SRC:CRYPTSETUP-2.8.6 cryptsetup-close]

[CLAIM:PFIR07-C020][CLASS:SOURCE][STATUS:SUPPORTED][SRC:CRYPTSETUP-2.8.6 cryptsetup-close]

### HaloKV process keys

Workers may hold `K_enc`, `K_id`, `K_manifest`, plaintext, decompressed data, or derived subkeys. Controls include:

* smallest feasible worker and key-broker privilege;
* no key material in arguments, environment dumps, logs, metrics, tracing, or panic text;
* disable or encrypt core dumps and swap according to platform policy;
* lock sensitive buffers where justified, while acknowledging OS limits;
* explicit zeroization using library/compiler-supported primitives;
* bounded key leases and process lifetime;
* process termination/restart for emergency revocation;
* exclusion of secret-bearing pages from backup/checkpoint mechanisms where supported.

These reduce retention; they do not prove every register, cache, device, or forensic copy is erased.

[CLAIM:PFIR07-C006][CLASS:SOURCE][STATUS:SUPPORTED][SRC:LINUX-FSCRYPT-7.2RC3 §Removing keys]

[CLAIM:PFIR07-C043][CLASS:SOURCE][STATUS:SUPPORTED][SRC:NIST-SP800-88R2 §5.4]

## Revocation completion criteria

A revocation is complete only for the stated layer and scope. Evidence should identify:

1. authority and key IDs/epochs revoked;
2. broker authorization disabled;
3. workers drained/terminated and leases expired;
4. fscrypt removal result, when applicable;
5. mount and dm-crypt mapping state, when applicable;
6. kernel keyring links removed, when applicable;
7. manifests/indexes invalidated and epoch floor advanced;
8. backup/escrow/snapshot dependencies and whether they remain;
9. quarantine/evidence retention;
10. residual memory and media limitations.

Never use “all keys erased” without an inventory-backed scope and qualification.
