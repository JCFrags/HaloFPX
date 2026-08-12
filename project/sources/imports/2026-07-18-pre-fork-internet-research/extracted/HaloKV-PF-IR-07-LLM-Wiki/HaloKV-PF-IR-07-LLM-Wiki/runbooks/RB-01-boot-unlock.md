# RB-01 — Boot, unlock, and cache readiness

## Preconditions

* Approved host/rank identity and workload principal.
* Current lower-layer configuration, key-broker trust, and policy hashes.
* Current tenant/project/prefix epoch floors and revoked-key list.
* Authoritative compute path can operate without persistent cache.
* No plaintext fallback cache path exists.

## Procedure

1. Start with the encrypted cache volume closed and persistent cache serving disabled.
2. Verify expected block-device identity, LUKS UUID/header location, and boot policy before unlock.
3. Acquire the LUKS unlock condition through the approved method. In unattended mode, do not silently fall back to interactive or an unprotected key file.
4. Activate the mapping and verify expected cipher/mode/device identity using the installed release's official tooling. Record status without collecting key bytes.
5. Mount with restrictive owner/mode and the approved filesystem options. Treat mount or integrity errors as cache unavailable.
6. If fscrypt is used, install only the required v2 directory policy key. Verify policy identity and deny plaintext alternate directories.
7. Authenticate the cache workload to the key/principal authority. Obtain only the authorized rank-epoch purpose keys or broker handles.
8. Load current epoch/generation floors, deletion tombstones, accepted format/suite policy, and public/system publisher trust.
9. Initialize nonce allocation. If state is missing, restored, cloned, or inconsistent, invoke RB-03 and create a new epoch before writes.
10. Authenticate the current manifest. Build no serving index from unauthenticated or stale metadata.
11. Run a negative self-test: wrong tag/AAD test object must return `MISS_RECOMPUTE` and expose no detailed external error.
12. Mark cache ready. If any step fails, remain cold and recompute; do not weaken validation.

## Evidence

* host/rank/principal IDs;
* volume UUID and mapping name/status;
* fscrypt policy/key identifier and removal-status baseline, if applicable;
* key IDs/epochs and broker authorization decision—not key material;
* nonce allocator instance/range high-water evidence;
* accepted manifest generation/hash;
* self-test result and cache-ready transition.

## Failure handling

* Missing automated volume key: boot may continue with persistent cache unavailable if policy permits.
* Wrong LUKS header or unexpected mapping: stop; quarantine/image only under incident authority.
* Key broker unavailable: cold cache; no local parent-key fallback.
* Manifest/tag/policy mismatch: miss/quarantine and rebuild; never parse or repair unauthenticated payload.

[CLAIM:PFIR07-C023][CLASS:SOURCE][STATUS:SUPPORTED][SRC:SYSTEMD-CRYPTTAB-261.1 §Description]

[CLAIM:PFIR07-C058][CLASS:SYNTHESIS][STATUS:REQUIRED][SRC:C003,C016,C030,C034]
