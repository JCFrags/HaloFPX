# fscrypt, LUKS/dm-crypt, and object AEAD

## Comparative conclusion

The three mechanisms enforce controls at different abstraction layers:

* **fscrypt** is a filesystem policy/key-domain mechanism. It is useful for selective offline confidentiality of directory trees and filename protection, but ordinary fscrypt content modes do not authenticate HaloKV objects.
* **dm-crypt/LUKS2** is a block-volume encryption and keyslot/unlock mechanism. It is the strongest operationally common stolen-volume baseline, but ordinary XTS mode is confidentiality-only and a shared unlocked volume does not separate tenants.
* **Object AEAD** is the only option that natively binds HaloKV object bytes to tenant/project/prefix/rank/epoch/format semantics and can implement the required miss/recompute contract.

[CLAIM:PFIR07-C045][CLASS:SOURCE][STATUS:SUPPORTED][SRC:NIST-SP800-111 §§2-4]

[CLAIM:PFIR07-C051][CLASS:SYNTHESIS][STATUS:RECOMMENDED][SRC:C004,C009-C012,C027,C039]

## Decision matrix

| Dimension | fscrypt v2 | dm-crypt/LUKS2 | Object AEAD | HaloKV decision |
| --- | --- | --- | --- | --- |
| Control granularity | Per encrypted directory tree / inode policy; file and filename encryption | Whole block mapping or volume | Per object/chunk/envelope and manifest | Use object AEAD as semantic enforcement; layer fscrypt/LUKS where operationally justified. |
| Primary threat boundary | Offline filesystem access without installed policy key | Offline block-device or volume access without active volume key | Unauthorized, wrong-domain, corrupt, substituted, or tampered cache object | These boundaries are complementary, not interchangeable. |
| At-rest payload confidentiality | Native for file contents and filenames within protected trees | Native for mapped blocks while mapping is closed; common XTS mode | Native for each encrypted payload | Require object AEAD; prefer at least one lower storage layer for stolen-media defense. |
| Cryptographic authenticity | Absent for ordinary file-content modes | Absent for ordinary XTS; possible with dm-integrity authenticated modes, selected stack marks experimental | Native tag verification over ciphertext and AAD | Only object AEAD supplies required object-semantic authentication in the minimum profile. |
| Tenant/project/principal binding | Indirect key-domain separation only; no HaloKV principal fields | Volume unlock identity only; no HaloKV principal fields | Explicit canonical principal/scope fields in AAD and KDF context | Bind authoritative principal IDs in AAD and authorization, not display names or mutable OS labels. |
| Wrong key / wrong tenant behavior | May appear as unavailable names/content; not a HaloKV miss contract | Mapping activation failure or garbage/I/O behavior depending mode | AEAD FAIL normalized to MISS_RECOMPUTE | One external miss class; precise reason only in access-controlled telemetry. |
| Unknown/corrupt object | Filesystem/application-dependent | May produce I/O errors only with supporting integrity; otherwise corruption may pass upward | Envelope parse bounds + tag + identifier + manifest checks; any failure is miss | Fail closed before decompression/deserialization. |
| Replay / rollback protection | None | None by default | Not supplied by AEAD alone; can bind epoch/generation but needs trusted freshness state | Maintain trusted epoch floors and manifest generations where freshness is required. |
| Online privileged compromise | Does not protect after keys are installed against privileged live compromise | Does not protect after mapping is active | Keys/plaintext available to authorized process; does not defeat compromised process/kernel | Out of cryptographic at-rest scope; use OS isolation, least privilege, attestation only as separate controls. |
| Online co-tenant separation | Possible directory-key separation plus OS access controls | Weak if tenants share one unlocked mapping | Strongest native cryptographic tenant/project binding if keys and AAD are separated | Object AEAD mandatory for shared multi-user cache. |
| Peer/rank separation | Possible separate directory policies but operationally coarse | Separate volumes/mappings are coarse | Per-rank derived key within epoch; rank ID in AAD | Use rank-local AEAD key domain and rank-local nonce state. |
| Nonce/IV lifecycle | Kernel/filesystem mode-specific; application does not manage object nonce | Sector-IV mode managed by mapping configuration | Explicit 96-bit nonce allocation and per-key uniqueness; epoch rollover on uncertain state | Persist monotonic allocation safely or use randomly generated nonces under strict limits; never reuse intentionally. |
| Key storage and unlock | Filesystem master keys installed through fscrypt key APIs/keyrings | LUKS2 keyslots/tokens unwrap volume key; boot via crypttab or orchestrator | KMS/HSM/wrapped epoch roots; rank keys delivered to least-privileged service memory | No plaintext long-lived rank key on cache media; record authority and key owner. |
| Boot behavior | Mount may proceed while protected names remain inaccessible until key installation | Mapping may block, fail, or be optional depending crypttab/orchestrator policy | Cache starts cold/read-disabled until key authorization; compute path remains authoritative | Boot must fail closed for cache serving but should degrade to recomputation rather than plaintext cache. |
| Busy-file/mapping revocation | Open/in-use files can retain per-file keys after master-key removal | Mapping close can fail while busy; deferred close is not immediate | Process-held keys and already decrypted buffers survive until drained/wiped/terminated | Drain, close/unmap, revoke, verify absence, then destroy key references; document incomplete eviction. |
| Rotation | New policy/key usually means migration/rewrite or new tree | Credential/keyslot rotation differs from payload reencryption; reencryption has recovery modes | New epoch for writes; old epoch read-only during bounded transition; lazy recompute preferred | Prefer epoch rollover and cache recomputation over bulk in-place transformation. |
| Revocation blast radius | Directory-tree policy key | Entire volume key / all data on mapping | Tenant/project/prefix/rank/epoch scope; per-object with independent DEKs if required | Choose hierarchy granularity according to deletion and incident requirements. |
| Crash recovery | Filesystem crash behavior; no application authenticity/freshness semantics | Filesystem plus mapping; dm-integrity journal and LUKS reencryption resilience matter | Immutable object write + atomic manifest publish; nonce state and epoch recovery are explicit | Treat uncertain nonce counters or partially published manifests as miss/new epoch. |
| Backup/restore | Backup needs keys/policies and may expose names/metadata depending tool | Raw volume plus protected LUKS header/key dependencies; header backups are sensitive | Coherent ciphertext, authenticated manifests, format version, wrapped epochs, policy metadata | No cache backup by default; quarantine and validate any restore. |
| Content addressing and dedup | Filesystem block/file dedup interactions are filesystem-specific and can leak equality | Below-filesystem encryption usually disrupts physical ciphertext dedup and reveals allocation via discard | Scoped keyed CID enables authorized logical dedup; randomized ciphertext itself is not the stable key | Private dedup ≤ tenant/project sharing scope; public/system namespace separate. |
| Cryptographic erasure | Potentially tree-scoped only if all master/derived copies become inaccessible; busy files complicate | Volume-scoped if all volume-key unlock paths, headers, backups, escrow, and snapshots are eliminated | Epoch- or object-scoped depending independent key wrapping; all copies and backups still matter | State scope and evidence; never claim physical zeroization. |
| Discard and physical sanitization | Filesystem discard behavior separate from encryption | Pass-through discard leaks allocation and does not prove erase | Logical deletion/key destruction does not command media sanitization by itself | Use NIST SP 800-88r2-aligned media-specific procedure with verification/validation. |
| Operational complexity | Moderate; policy inheritance/keyring/busy-file caveats | Moderate; boot dependencies, mappings, header backup, reencryption | Highest implementation responsibility; envelope, canonicalization, nonce state, key authority, telemetry | Complexity is justified because semantic tenant/object binding cannot be delegated to lower layers. |
| Recommended HaloKV role | Optional/conditional defense-in-depth for per-tenant or service directory isolation | Recommended host-volume baseline where operationally supported | Required minimum multi-user persistent-cache control | Profile: object AEAD + optional fscrypt v2 + LUKS2/dm-crypt at host-volume layer. |

## fscrypt threat boundary and caveats

fscrypt protects file contents and filenames within protected trees against an offline observer who lacks the key, while leaving most filesystem metadata visible. The kernel documentation explicitly does not provide a general authenticity guarantee against offline modification. Once the key is installed, OS access control is the live protection boundary.

[CLAIM:PFIR07-C002][CLASS:SOURCE][STATUS:SUPPORTED][SRC:LINUX-FSCRYPT-7.2RC3 §Threat model]

[CLAIM:PFIR07-C003][CLASS:SOURCE][STATUS:SUPPORTED][SRC:LINUX-FSCRYPT-7.2RC3 §Threat model]

Key removal is not instantaneous revocation for active objects: open or otherwise in-use files can retain per-file keys, and userspace must erase its own copies. Even after removal, this is not a claim that all plaintext copies, VFS state, registers, or caches have been wiped.

[CLAIM:PFIR07-C005][CLASS:SOURCE][STATUS:SUPPORTED][SRC:LINUX-FSCRYPT-7.2RC3 §Removing keys]

[CLAIM:PFIR07-C007][CLASS:SOURCE][STATUS:SUPPORTED][SRC:LINUX-FSCRYPT-7.2RC3 §Memory wiping]

**Configuration posture:** use fscrypt v2 policies for new use; isolate the cache process and key installer; keep policy descriptors and authority mappings in authenticated HaloKV metadata; rehearse key removal with busy descriptors. Do not use filesystem path or directory key possession as the sole tenant authorization decision.

## dm-crypt/LUKS2 threat boundary and caveats

LUKS2 stores and wraps a volume key through multiple credentials/keyslots and optional token metadata. Ordinary dm-crypt with XTS-AES provides block confidentiality. It does not authenticate blocks, objects, tenants, manifests, or freshness. LUKS2 metadata checksums/redundancy support corruption recovery but are not a general malicious-header authentication boundary.

[CLAIM:PFIR07-C010][CLASS:SOURCE][STATUS:SUPPORTED][SRC:LUKS2-SPEC-1.1.4 §Introduction]

[CLAIM:PFIR07-C011][CLASS:SOURCE][STATUS:SUPPORTED][SRC:LUKS2-SPEC-1.1.4 §§1.1,2]

[CLAIM:PFIR07-C039][CLASS:SOURCE][STATUS:SUPPORTED][SRC:NIST-SP800-38E §1]

`dm-integrity` can pair data with integrity tags and journaling, and authenticated dm-crypt modes can turn detected tampering into I/O failure. In the selected release documentation this path remains an optional/experimental configuration, and it still does not know HaloKV tenant or object semantics. Evaluate only after fault injection, performance testing, and recovery drills.

LUKS header backups, detached headers, alternate keyslots, tokens, and escrow are key-bearing dependencies. Closing an active mapping fails while it is busy; deferred close is a future action, not completed revocation. Passphrase/keyslot rotation does not necessarily change the payload volume key.

[CLAIM:PFIR07-C014][CLASS:SOURCE][STATUS:SUPPORTED][SRC:CRYPTSETUP-2.8.6 cryptsetup-luksHeaderBackup]

[CLAIM:PFIR07-C019][CLASS:SOURCE][STATUS:SUPPORTED][SRC:CRYPTSETUP-2.8.6 cryptsetup-close]

[CLAIM:PFIR07-C022][CLASS:SOURCE][STATUS:SUPPORTED][SRC:CRYPTSETUP-2.8.6 luksChangeKey and reencrypt]

## Object AEAD responsibility

The application owns more correctness obligations: canonical encoding, key hierarchy, nonce allocation, format versioning, parser bounds, replay state, atomic publish, telemetry, and negative tests. That complexity is accepted because lower layers cannot enforce the HaloKV semantic boundary.

The object layer **does not eliminate the value** of lower layers. A practical profile is:

1. Rank-local LUKS2/dm-crypt volume, unlocked only for the cache service lifecycle.
2. Optional fscrypt v2 tree separation where selective unlock or host-user separation is useful.
3. Mandatory object AEAD and authenticated manifests inside those layers.
4. Optional dm-integrity authenticated storage only after environment-specific validation.

## Unsupported substitutions

[CLAIM:PFIR07-C071][CLASS:UNSUPPORTED][STATUS:REJECTED][SRC:C003,C009]

[CLAIM:PFIR07-C072][CLASS:UNSUPPORTED][STATUS:REJECTED][SRC:C011-C012,C039]

[CLAIM:PFIR07-C081][CLASS:UNSUPPORTED][STATUS:REJECTED][SRC:C030-C049]
