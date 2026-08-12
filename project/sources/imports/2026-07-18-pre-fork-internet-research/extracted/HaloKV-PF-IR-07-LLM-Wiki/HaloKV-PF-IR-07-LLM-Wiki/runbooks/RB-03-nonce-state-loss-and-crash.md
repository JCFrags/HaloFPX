# RB-03 — Nonce-state loss, crash, clone, or snapshot restore

## Trigger

Use this runbook whenever the system cannot prove which nonces were consumed
under a specific AEAD key, including allocator corruption, rollback, restored
snapshot, cloned rank, failed durability acknowledgement, or concurrent allocator
identity.

## Safety rule

> Do not reconstruct a next counter from visible object files and continue the
> same key. Create a fresh epoch key before any new encryption.

## Procedure

1. Stop writes for the affected `(authority, tenant, project, prefix, namespace, rank, epoch, key_id)` domain.
2. Preserve allocator records, object/manifest hashes, host snapshot identity, and key-broker audit for analysis.
3. Mark the old epoch `NONCE_STATE_UNCERTAIN` and deny further seal operations. Reads may continue only if incident policy permits and each object fully authenticates; suspected reuse normally triggers compromise handling.
4. Allocate a new unique rank/allocator instance if a clone or restore occurred.
5. Generate a fresh epoch root and purpose keys; initialize a new counter/random-nonce budget.
6. Burn any previously reserved but unused ranges. Never return them to the old key domain.
7. Advance active epoch/generation and publish an authenticated manifest policy.
8. Recompute or authenticated-import old objects into the new epoch. Do not copy ciphertext as a new object.
9. Monitor for duplicate `(key_id, nonce)` pairs and investigate any finding as a key incident.
10. Close with root-cause, durability guarantees, restored-state path, and tests added.

## Crash-point checks

* reservation record durable before nonce use;
* no range reuse after process death;
* atomic immutable object publish;
* manifest/index published last;
* restored backup never resumes old key/counter;
* two ranks/clones cannot share allocator instance under one key.

[CLAIM:PFIR07-C031][CLASS:SOURCE][STATUS:SUPPORTED][SRC:NIST-SP800-38D §8]

[CLAIM:PFIR07-C033][CLASS:SOURCE][STATUS:SUPPORTED][SRC:NIST-SP800-38D §8.2.1]
