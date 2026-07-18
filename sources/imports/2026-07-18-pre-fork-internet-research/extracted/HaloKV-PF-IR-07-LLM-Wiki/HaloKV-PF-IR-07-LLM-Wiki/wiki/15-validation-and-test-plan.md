# Validation and adversarial test plan

## Cryptographic conformance

* Run official/known-answer tests for every enabled AEAD, HMAC, KDF, and deterministic-encoding implementation.
* Test 128-bit tag enforcement and reject shorter, missing, or extra tag material.
* Cross-check AAD bytes and scoped CID bytes across every supported implementation language.
* Verify that suite/key IDs are authenticated and that unknown critical fields fail closed.
* Confirm key-derivation contexts change output for each tenant, project, prefix, sharing class, rank, epoch, and purpose.
* Confirm the object process cannot request parent or sibling key domains from the broker.

## Negative object corpus

For each valid object, generate separate cases with:

1. one-bit ciphertext modification;
2. one-bit tag modification;
3. nonce change;
4. each AAD field changed independently;
5. wrong tenant/project/prefix/rank/epoch/key reference;
6. stale manifest generation and tombstoned object;
7. CID mismatch after valid decryption;
8. truncation at every structural boundary;
9. oversized and integer-overflowing lengths;
10. duplicate map keys, non-canonical ordering, invalid UTF-8 where disallowed, unknown critical extensions;
11. decompression bomb and parser-recursion inputs behind a valid tag from a test key;
12. object copied to another path or manifest entry.

Every invalid case must return `MISS_RECOMPUTE` to the caller. The test harness separately verifies the protected internal reason code without exposing it through the public response.

## Nonce and crash tests

* Kill the process before and after nonce-range durability, during encryption, after file sync, during rename, and during manifest publish.
* Restore VM/filesystem snapshots at each point and verify a fresh epoch is required when allocator state is uncertain.
* Run concurrent writers and prove no overlap in reserved ranges.
* Clone a rank and prove the clone cannot retain the same allocator identity/key domain.
* Force the local invocation ceiling and verify writes stop/rotate before another nonce is issued.
* Scan produced objects for duplicate `(key_id, nonce)` pairs as a monitoring backstop, not the primary guarantee.

## Lower-layer tests

When LUKS2/dm-crypt is used:

* wrong token/passphrase, missing key service, headless boot, nofail/cold-cache behavior;
* busy mapping close and deferred-close observation;
* header backup/restore only against an identity-verified test volume;
* keyslot removal while mapping is active to demonstrate non-immediate payload-key eviction;
* interrupted reencryption under each selected resilience configuration;
* discard enabled/disabled behavior and documented leakage decision.

When fscrypt is used:

* v2 policy inheritance and wrong-directory policy;
* master-key removal with closed files and with open descriptors, mmap, cwd/root references;
* userspace key-buffer zeroization and worker termination;
* boot without fscrypt key yielding cold/unavailable cache, not plaintext fallback.

When dm-integrity/authenticated dm-crypt is evaluated:

* data/tag corruption, journal replay/recovery, direct-mode power failure, performance, and I/O error propagation;
* object AEAD remains enabled so a lower-layer success cannot bypass semantic checks.

## Backup, restore, deletion, and incident tests

* Restore coherent backup and deliberately omit each required component.
* Restore an old but valid generation below current floor.
* Restore an object covered by a deletion tombstone.
* Restore old nonce state and verify new epoch/key before writes.
* Attempt direct publication from quarantine and require denial.
* Remove one LUKS keyslot while alternate slots/header backups exist and verify no CE claim is emitted.
* Delete an object sharing an epoch key and verify the report says logical deletion, not per-object CE.
* Exercise KMS destroy with a deliberately retained backup dependency; validation must fail CE completion.
* Run media-sanitization workflow with success, unsupported device command, verification failure, and validation rejection.
* Simulate key compromise, public publisher compromise, wrong-principal mapping, and cross-tenant index exposure.

## Side-channel and abuse tests

Measure hit, unknown object, wrong key, bad tag, stale generation, and missing-key paths from each caller boundary. Exact constant time for a storage/cache service is generally unrealistic; the requirement is that the API shape and authorization/index partition do not expose a simple cross-tenant oracle. Test per-tenant quota isolation, invalid-object floods, recomputation coalescing, and memory/CPU bounds.

## Acceptance evidence

A release record should retain:

* cryptographic library/module versions and build provenance;
* enabled algorithm suites and format versions;
* KDF/AAD/CID canonicalization test-vector hashes;
* fault-injection matrix results;
* key-broker authorization tests;
* boot/unlock and busy-revocation drill results;
* backup/restore/quarantine drill results;
* deletion/CE/media report examples reviewed for accurate wording;
* residual-risk sign-off and human decisions;
* bundle/source manifest verification result.

Passing this plan supports the local HaloKV profile. It is not a certification or compliance determination.
