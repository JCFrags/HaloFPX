---
type: research-follow-up
status: proposed
target: "HaloKV encryption and key lifecycle"
created: 2026-07-16
last_verified: 2026-07-17
risk: high
approval_required: human
decision: defer
related_requirements: ["FR-CAC-003", "FR-CAC-004", "FR-SEC-002"]
related_sections: ["57", "58", "59", "60", "63", "64", "72"]
---

# HaloKV encryption and key-lifecycle research

## Review decision

**DEFER implementation pending an ADR, a two-host TPM/systemd capability inventory, and destructive recovery/rotation tests.** The design below is a **[RECOMMENDATION]**, not an approved cryptographic profile. Current evidence supports the architecture and failure boundaries, but it does not establish the exact TPM capabilities, systemd version, FIPS policy, cryptographic-library support, or recovery objective on the two target Strix Halo hosts.

The safest initial operating posture is **[RECOMMENDATION]**: treat cached state as recomputable; do not back up cache ciphertext or data-encryption keys by default; encrypt the cache volume for powered-off media protection; and add application/object encryption before enabling persistent private multi-user state. Application encryption is required for per-tenant revocation and for keeping one tenant's plaintext-derived identifiers from becoming a cross-tenant oracle. Volume encryption alone does not provide those properties.

## Scope and evidence boundary

This report resolves research backlog item NR-04 at the review layer. It does not edit or promote Wiki claims. It uses the existing Section 57 compatibility hierarchy, Section 63 immutable-generation commit protocol, and Section 64 identity-isolation rule as candidate design inputs. Those sections remain `needs-machine-validation`.

The review method follows the canonical Agent Harness routed by [`references/agent-harness.md`](../../references/agent-harness.md): preserve provenance, keep unsupported design in review, record an explicit accept/revise/defer/reject decision, and review the review itself before closeout. The Agent Harness is methodological authority here, not evidence for cryptographic behavior.

Claim labels are literal:

- **[VERIFIED]** means the cited primary source directly supports the statement.
- **[INFERENCE]** means the statement follows from multiple cited facts but is not specified by them.
- **[RECOMMENDATION]** is a HaloKV design proposal requiring approval and validation.
- **[OPEN]** is unresolved.

## Threat model

| Threat actor or failure | Security objective | Expected behavior | Boundary or residual risk |
|---|---|---|---|
| Another authenticated local user or unprivileged process | Tenant confidentiality and authorization | No cross-tenant lookup, metadata oracle, plaintext, or usable key | Root, kernel, and a compromised service are outside this isolation boundary |
| Stolen powered-off host, NVMe device, or copied cache directory | Confidentiality and integrity at rest | Ciphertext and wrapped keys reveal no cache plaintext; mutation is detected | A running unlocked host can expose plaintext and keys |
| Compromise of one rank | Limit peer blast radius | Host-unlock material is host-specific; peer cannot copy a TPM-sealed host secret and unseal it elsewhere | The domain/scope keys delivered to a compromised live rank expose scopes that rank is authorized to serve |
| Passive or active USB4/TCP-path attacker | Rank authentication, confidentiality, integrity, replay resistance | Reject unauthenticated peer, modified control message, stale epoch, or substituted envelope | Traffic length/timing remain visible; endpoint compromise remains decisive |
| Offline ciphertext, manifest, or index tampering | No silent corruption or rollback | Authentication failure is `MISS_RECOMPUTE` or quarantine, never accepted state | Availability attacks can still delete or corrupt cache data |
| Crash, power loss, full disk, or restart during write/rotation | Recover last complete valid generation | Ignore uncommitted objects; resume rotation idempotently; never reuse an unsafe nonce | Hardware durability semantics still require on-machine testing |
| Operator mistake, TPM replacement, firmware/PCR change, or lost key | Controlled recovery or deliberate cache loss | Restore only from an approved recovery package; otherwise cold recompute | No recovery package means encrypted cache loss by design |
| Deleted tenant or expired retention | Prompt logical revocation and bounded key destruction | Stop lookup immediately; retire scope key across both ranks and retained generations | Physical erasure is not guaranteed while ciphertext/key copies exist in backups, snapshots, memory, peer storage, or SSD remapping |

**[RECOMMENDATION]** Explicit non-goals for the first profile: protection from a malicious root/kernel/hypervisor; secrets after live-service compromise; DRAM/GPU-memory extraction, cold-boot or DMA attacks; malicious firmware/TPM; and traffic-analysis resistance. These require platform hardening beyond cache encryption. Encryption must not replace authenticated service identities, restrictive filesystem permissions, compatibility validation, or corruption-as-miss behavior.

## Primary-source findings

1. **[VERIFIED]** NIST SP 800-57 Part 1 Rev. 5 defines general protection, lifecycle, inventory, backup, recovery, compromise, and access-control concerns for cryptographic keying material [KL-01]. It supports a managed key hierarchy and lifecycle record; it does not choose a HaloKV envelope format.
2. **[VERIFIED]** NIST SP 800-38D specifies GCM authenticated encryption with associated data [KL-02]. AES-GCM requires a nonce-management design that prevents reuse under a key. The publication is final but under revision, so the exact approved profile must be rechecked at implementation freeze.
3. **[VERIFIED]** NIST SP 800-38F specifies AES-KW and AES-KWP for confidentiality and integrity of cryptographic keys [KL-03]. It is suitable for wrapping randomly generated HaloKV DEKs; its current revision work must be monitored.
4. **[VERIFIED]** NIST SP 800-108 Rev. 1 specifies HMAC-, CMAC-, and KMAC-based key derivation [KL-04]. Derived subkeys require labels/context and domain separation; a generic hash concatenation is not a substitute for a reviewed KDF.
5. **[VERIFIED]** NIST SP 800-88 Rev. 2 defines cryptographic erase as sanitizing the key or keys that provide confidentiality for target data, making plaintext recovery infeasible for a specified effort [KL-05]. It also treats media sanitization as a program with assurance and verification, not as a file-delete synonym.
6. **[VERIFIED]** TPM 2.0 provides protected objects, authorization policies, PCR-bound policy mechanisms, and protected storage facilities; the current TCG Library revision is Version 185, March 2026 [KL-06]. A TPM is appropriate for sealing a small host-unlock secret, not for bulk cache encryption.
7. **[VERIFIED]** systemd source documentation at commit `8009fa49845cd6fb7b7014ab06218b68fe702006` states that encrypted credentials can use a TPM-derived key, a host key, or both; `host+TPM2` is the normal automatic choice when both are available. `LoadCredentialEncrypted=` authenticates/decrypts before passing a credential to the service, and decryption failure fails the service [KL-07][KL-08]. This is a delivery mechanism, not a complete distributed key manager.
8. **[VERIFIED]** `systemd-cryptenroll` enrolls TPM2 and recovery methods for LUKS2 and can bind unsealing to PCR state [KL-09]. LUKS2 protects a volume at rest but exposes plaintext uniformly after unlock; it cannot by itself implement tenant-scoped revocation.
9. **[VERIFIED]** Linux keyrings provide ownership, permissions, expiry, revocation, and process/session scopes [KL-10]. They can reduce ordinary userspace exposure of runtime secrets, but do not establish durable authority and do not protect against privileged live-host compromise.
10. **[VERIFIED]** TLS 1.3 provides authenticated, confidential, integrity-protected channels; RFC 9846 supersedes RFC 8446 while retaining TLS 1.3 compatibility [KL-11]. The application protocol must still define peer identity verification and replay/epoch semantics.
11. **[VERIFIED]** RFC 8452 defines AES-GCM-SIV, a nonce-misuse-resistant AEAD, but it is an IRTF Informational RFC rather than an IETF Standards Track or NIST-approved mode [KL-12]. It is therefore an alternative only if the project's compliance policy and implementation support explicitly permit it.

## Recommended envelope and key hierarchy

```text
offline recovery KEK (normally absent from hosts)
  `-- wraps recovery copies of cache-domain/scope KEKs

per-host TPM/system credential
  `-- unlocks one host-unwrapping key (different on rank 0 and rank 1)
       `-- unwraps the cache-domain KEK copy authorized for that host
            `-- wraps tenant/public scope KEKs
                 `-- wraps random DEKs for immutable segment/object generations
                      `-- AEAD-encrypts cache payload and authenticates its header
```

| Key/material | Scope and lifetime | Storage | Rotation purpose |
|---|---|---|---|
| Recovery KEK | Administrative recovery domain; offline | Hardware token or separately protected offline package; never a normal host file | Recover after TPM/host replacement only when recovery is required |
| Host-unwrapping key | One physical host | Sealed/delivered by TPM-backed `systemd-creds`, or a directly sealed TPM object | Re-enroll a host without copying its TPM secret to the peer |
| Cache-domain KEK | One approved two-rank trust domain | One independently host-wrapped copy per rank; optional recovery-wrapped copy | Move domain authority without re-encrypting payloads |
| Scope KEK | One opaque tenant/project scope, plus explicitly public scopes | Wrapped by cache-domain KEK; never shared across private tenants | Tenant revocation, selective rotation, cryptographic-erasure boundary |
| DEK | One immutable segment/object generation | Random 256-bit key wrapped by scope KEK using AES-KWP | Limit nonce/key usage and permit KEK-only rewrap |

**[RECOMMENDATION]** Do not use one cluster-wide plaintext key file, derive tenant keys directly from tenant names, copy a TPM-sealed blob between ranks, or encrypt every tenant with the same scope key. Private scopes should not cross-deduplicate. Public/shared prefix scopes require an explicit policy and must never contain tenant-derived continuations.

### Object envelope candidate

**[RECOMMENDATION]** Each immutable object stores a bounded, versioned envelope containing:

```yaml
envelope_schema: halokv.envelope.v1
object_id: opaque-scope-keyed-id
scope_id: opaque-authenticated-principal-or-public-scope
semantic_compat_root: sha256:...
topology: {world_size: 2, logical_rank: 1, ownership_digest: sha256:...}
payload_suite: AES-256-GCM
nonce_strategy: random-96-v1
nonce: base64:...
key_wrap_suite: AES-256-KWP
scope_key_id: opaque-id
scope_key_epoch: 7
wrapped_dek: base64:...
ciphertext_length: 123456
ciphertext_digest: sha256:...
```

AEAD associated data must be the deterministic canonical encoding of the envelope's immutable identity and policy fields: envelope schema, object/segment ID, opaque scope ID, semantic compatibility root, object type, topology/world size/logical rank/ownership digest, payload suite, nonce strategy, key-wrap suite, scope-key ID/epoch, and plaintext length. The wrapped-key plaintext must also carry a digest of this context so a valid wrapped DEK cannot be substituted between envelopes.

**[RECOMMENDATION]** Baseline payload protection is AES-256-GCM from a maintained cryptographic library, using a fresh random 96-bit nonce and a fresh random DEK per immutable segment/object generation. Never retry publication with the same DEK/nonce after an uncertain crash; discard that DEK and allocate a new generation. Enforce per-key invocation/byte limits from the selected library/profile. AES-256-GCM-SIV is a candidate only if a later ADR accepts its non-NIST, Informational-RFC status and pinned-library support.

**[RECOMMENDATION]** Release no plaintext until AEAD verification succeeds. Any authentication, digest, canonicalization, scope, compatibility, topology, or key-context failure is a miss/quarantine event. It is never a partial restore and never repaired optimistically.

## Compatibility fingerprints and privacy-preserving identity

Encryption lifecycle must not redefine inference compatibility.

- **Semantic compatibility root:** Keep Section 57's domain-separated SHA-256 over deterministic canonical data. It includes model, tokenizer/template, math/numeric state, runtime ABI, topology ownership, and object semantics. It excludes DEK/KEK IDs, key epochs, nonce, ciphertext, and physical host-unlock method.
- **Storage-envelope fingerprint:** Hash the canonical envelope schema, algorithms, nonce strategy, wrapped-DEK format, key ID/epoch, ciphertext length/digest, and AAD-schema version. Rewrapping or encryption-suite migration changes this fingerprint and storage generation, not semantic identity.
- **Committed manifest binding:** The generation manifest binds the semantic root and storage-envelope fingerprint for every required rank object. A valid semantic root with an invalid envelope is still a miss.
- **Private lookup identity:** **[RECOMMENDATION]** Use a scope-keyed PRF/HMAC over the canonical plaintext content identity for private objects. Store any unkeyed plaintext content digest inside authenticated encrypted metadata. This prevents a shared global digest index from revealing cross-tenant equality. Public scopes may use an unkeyed content digest only after policy approval.

## TPM and system-credential options

| Option | What it establishes | Important limitation | Recommendation |
|---|---|---|---|
| LUKS2 + TPM2/recovery enrollment | Powered-off volume confidentiality and boot unlock | After unlock it is a shared plaintext filesystem; no per-tenant revocation | **Use as defense in depth**, not as the HaloKV envelope authority |
| `systemd-creds` `host+TPM2` + `LoadCredentialEncrypted=` | Host-bound authenticated delivery of a small unlock credential to the service | Secret exists in service memory after delivery; OS/TPM replacement and PCR policy need recovery | **Preferred first host-unlock integration** if target systemd supports the required options |
| Direct TPM sealed object/policy | Fine-grained authorization/PCR policy and non-exportable parent protection | More code, TPM-session/security review, firmware variance, lockout and update risk | Defer until systemd credentials prove insufficient; seal only a KEK/unlock secret |
| Linux keyring | Permissioned, expiring, revocable runtime retention | Not durable authority; privileged process/kernel can recover or misuse live keys | Optional runtime cache for short-lived unwrapped keys |

**[RECOMMENDATION]** Also disable core dumps for the service, restrict debugging/ptrace, minimize privileges, encrypt swap or prevent key-bearing pages from swapping where the implementation can prove it, zero temporary key buffers using library-supported primitives, and bound decrypted-key residency. These reduce exposure but do not create a secure-deletion guarantee for RAM.

## Rank coordination and transport

**[RECOMMENDATION]** Protect rank-control and key-distribution traffic with mutually authenticated TLS 1.3 conforming to RFC 9846. Pin each logical rank to an approved certificate/public-key identity and bind that identity to cluster ID, host ID, logical rank, and current topology plan. Do not treat physical USB4 adjacency as authentication. Disable 0-RTT for key lifecycle and rotation operations.

The coordinator owns the monotonically increasing cluster key epoch and the durable rotation journal. Both ranks independently validate the exact semantic/storage schema, active and readable epochs, and their host-wrapped domain-key copy. A rank never accepts an epoch solely because its peer announced a larger integer.

With only two ranks, **[RECOMMENDATION]** do not claim consensus. Distributed epoch activation requires acknowledgements from both ranks and a durable coordinator commit. If a rank is absent, remain on the prior epoch or enter an explicitly configured single-node cold/recompute mode with a new topology identity; do not create distributed state under a partially activated epoch. A restored/stale rank must reauthenticate, read the committed journal, prove possession of its host-wrapped key, and catch up before serving cache hits.

## Rotation and revocation state machine

**[RECOMMENDATION]** Use append-only, idempotent states:

1. `PREPARED`: Generate a new KEK, key ID, and epoch. Write independently host-wrapped copies plus optional offline recovery copy; sync; do not use for writes.
2. `DISTRIBUTED`: Each rank unwraps/tests the new key and reports an authenticated acknowledgement containing cluster, rank, topology, key ID, epoch, and journal digest. Do not log key bytes.
3. `ACTIVATED`: Coordinator atomically commits the epoch after both acknowledgements. New objects use the new epoch; readers accept explicitly listed old readable epochs.
4. `REWRAPPING`: Rewrap existing DEKs into new side-by-side envelopes. Do not rewrite payload ciphertext and do not overwrite an old valid envelope.
5. `VERIFIED`: Inventory proves every live manifest has a valid new envelope on every required rank; a restore sample and full metadata scan pass. Recovery packages and retained backups are reconciled.
6. `RETIRED`: Remove old epoch from new-read lookup, then from live memory. Keep it only for a declared rollback window.
7. `DESTROYED`: Sanitize all authorized old-key copies after the rollback/backup retention period and record evidence. A missing rank, snapshot, or backup blocks a cryptographic-erasure claim.

Scope-key revocation immediately removes the scope from authorization and lookup before key destruction. DEK/algorithm rotation is different: write and verify new ciphertext in a new object generation, atomically switch the manifest, and retain the old generation only under bounded rollback policy. Never perform in-place ciphertext conversion.

## Crash recovery and rollback resistance

**[RECOMMENDATION]** Apply Section 63's immutable two-phase generation protocol to key metadata:

- Write new key records, host-wrapped copies, envelopes, and journal entries to new names; authenticate, sync, rename, then sync the directory.
- A small committed journal record selects the active key epoch. The highest filename or wall-clock timestamp is not authority.
- Restart replays idempotent states and chooses the highest *committed, authenticated, fully available* epoch whose predecessor chain and both-rank acknowledgements validate.
- Incomplete `PREPARED`, `DISTRIBUTED`, or `REWRAPPING` work is resumed or garbage-collected only after reachability checks. Old readable keys remain until verified migration completes.
- A suspected rollback, unknown key epoch, missing required envelope, AEAD failure, or cross-rank disagreement disables the affected cache hit and triggers recomputation/quarantine, never fallback to unauthenticated plaintext.

**[OPEN]** Whether to anchor rare key-epoch rollback detection in TPM NV state. TPM NV counters can complicate restore, replacement, and write endurance; they must not be incremented per object. Validate a journal/hash-chain solution first, then test TPM anchoring only for low-frequency administrative epoch commits.

## Backup and recovery

**[RECOMMENDATION]** The default recovery objective for cache data is zero: back up configuration, schemas, rotation audit records, and reproducibility metadata, but recompute cache payloads. This avoids turning disposable cache into a durable secret archive.

If a human-approved recovery objective requires cache recovery:

- Back up only ciphertext, authenticated manifests, and wrapped domain/scope keys; never plaintext DEKs, plaintext scope keys, or a host-unsealed credential.
- Protect recovery copies under an offline recovery KEK or hardware token under separate administrative custody. Record owners, inventory, retention, and restore authorization.
- Test restore onto a replacement host in a disposable environment: verify recovery authorization, enroll a new host-unwrapping key, restore only declared scopes, validate every envelope, and prove incompatible/corrupt data misses.
- Rotate recovery material after use and after any suspected exposure. A restore test that reads only metadata is insufficient.
- Include snapshots, exports, retired ranks, and offsite copies in rotation and deletion inventories. If any retained copy can recover an old scope key, the old ciphertext is not cryptographically erased.

## Secure-deletion limits

**[VERIFIED]** NIST SP 800-88 Rev. 2 distinguishes cryptographic erase from broader media-sanitization program assurance [KL-05]. Therefore:

- **Logical deletion** makes a tenant/object unreachable and denies future lookup. It does not erase bytes.
- **Key revocation** denies authorized use of a key. It does not prove every live or copied key byte is gone.
- **Cryptographic erase** is claimable only when the confidentiality key path for the target ciphertext has been sanitized everywhere required and the encryption was effective before the data was written.
- **Filesystem discard/TRIM** is an allocation hint, not a standalone proof of target-data sanitization.
- **Physical-media sanitization** is an administrative device-reuse/disposal action using an approved method with verification evidence.

**[RECOMMENDATION]** HaloKV user-facing APIs should say `deleted` or `logically revoked`, not `securely erased`. An administrative report may say `cryptographic erase completed` only after it inventories both ranks, runtime processes/keyrings, recovery material, snapshots/backups, retired manifests and rollback generations, and records the sanitization result. SSD wear leveling, remapping, page cache, swap, crash dumps, DRAM/GPU memory, and unavailable backup media prevent a claim of immediate physical erasure.

## Validation and release gates

No implementation should be promoted until the following evidence exists under `experiments/` with exact hardware, firmware, kernel, systemd, cryptographic-library versions, commits, configuration, and raw logs:

1. Inventory TPM manufacturer/firmware/capabilities, Secure Boot/measured-boot state, systemd credential options, cryptographic providers/FIPS mode, LUKS2 layout, swap/core-dump policy, and both-rank identities.
2. Demonstrate host-bound credential unseal on both nodes; prove copying the encrypted credential to the peer does not reveal the secret; exercise OS update/PCR change, TPM clear/replacement, and recovery path.
3. Run known-answer and negative tests for envelope canonicalization, AAD binding, AES-KWP unwrap, AEAD mutation/truncation, wrong scope/key/epoch/rank/topology, replay, and nonce uniqueness under concurrent writers and crash/retry.
4. Kill/power-cycle at every rotation state and filesystem durability boundary. Prove restart selects only a complete committed epoch and remains idempotent.
5. Partition the link and independently restart ranks during activation/rewrap. Prove no split epoch is used and single-node fallback changes topology and recomputes.
6. Rotate domain and scope KEKs without bulk payload rewrite; rotate a DEK/algorithm with side-by-side ciphertext; verify rollback, retirement, and old-key removal after retention.
7. Restore onto a replacement host from the approved package, then run a deliberately incomplete-package failure. Prove failure is cold recompute, not partial plaintext recovery.
8. Exercise tenant deletion with active leases, mmap/page cache, service restart, snapshots/backups, an offline rank, and expired rollback generations. Report exactly which deletion/erasure claim is supported.
9. Measure encryption/decryption, wrapping, cache-hit validation, rotation, and recovery overhead on matched cold/warm workloads. Performance never overrides an integrity or isolation failure.

Release fails on nonce reuse, unauthenticated plaintext release, cross-scope equality leakage, tenant/rank key substitution, accepted corruption, rollback to an uncommitted epoch, split-rank activation, unrecoverable required authority without an approved cold path, or any claim stronger than the deletion evidence.

## Open decisions for the ADR

1. Is cache data deliberately disposable, or is any tenant/public scope required to survive loss of both hosts?
2. What exact local adversary is in scope: unprivileged users only, service compromise, root, or physical live-memory access?
3. Is FIPS validation required? This decides whether AES-GCM-SIV can be considered and which provider/library build is acceptable.
4. What are the measured TPM/systemd capabilities and PCR-update/recovery behavior of both target machines?
5. Is cross-user public-prefix sharing allowed, and who is authorized to certify a prefix as public?
6. What rollback window and backup retention are allowed for each scope, and who can authorize recovery-key use?
7. Does deployment require two-rank availability during key rotation, or can rotation wait while one rank is unavailable?

## Source register

| ID | Primary source, pinned revision/date | Supports | Limitation |
|---|---|---|---|
| KL-01 | NIST, [SP 800-57 Part 1 Rev. 5](https://csrc.nist.gov/pubs/sp/800/57/pt1/r5/final), May 2020; accessed 2026-07-17 | Key-management protection, lifecycle, inventory, backup/recovery | General guidance; Rev. 6 work may change future profile |
| KL-02 | NIST, [SP 800-38D](https://csrc.nist.gov/pubs/sp/800/38/d/final), November 2007; accessed 2026-07-17 | AES-GCM AEAD | Under revision; does not design HaloKV nonce persistence |
| KL-03 | NIST, [SP 800-38F](https://csrc.nist.gov/pubs/sp/800/38/f/final), December 2012; accessed 2026-07-17 | AES-KW/KWP | Revision work is active |
| KL-04 | NIST, [SP 800-108 Rev. 1, Update 1](https://csrc.nist.gov/pubs/sp/800/108/r1/upd1/final), February 2024; accessed 2026-07-17 | PRF-based KDF and domain-separated context | Does not select a HaloKV KDF API |
| KL-05 | NIST, [SP 800-88 Rev. 2](https://csrc.nist.gov/pubs/sp/800/88/r2/final), September 2025; accessed 2026-07-17 | Cryptographic erase and media-sanitization assurance | Organization must choose and verify applicable method |
| KL-06 | TCG, [TPM 2.0 Library Specification Version 185](https://trustedcomputinggroup.org/resource/tpm-library-specification/), March 2026; accessed 2026-07-17 | TPM protected objects, policies, storage, current revision | Target TPM implementation/certification not inventoried |
| KL-07 | systemd project, [`systemd-creds` source manual](https://github.com/systemd/systemd/blob/8009fa49845cd6fb7b7014ab06218b68fe702006/man/systemd-creds.xml), commit `8009fa49845cd6fb7b7014ab06218b68fe702006`, inspected 2026-07-17 | TPM/host/combined encrypted-credential modes | Deployed version must still be inventoried |
| KL-08 | systemd project, [`systemd.exec` source manual](https://github.com/systemd/systemd/blob/8009fa49845cd6fb7b7014ab06218b68fe702006/man/systemd.exec.xml), commit `8009fa49845cd6fb7b7014ab06218b68fe702006`, inspected 2026-07-17 | `LoadCredentialEncrypted=` delivery and failure behavior | Live service version unknown |
| KL-09 | systemd project, [`systemd-cryptenroll` source manual](https://github.com/systemd/systemd/blob/8009fa49845cd6fb7b7014ab06218b68fe702006/man/systemd-cryptenroll.xml), commit `8009fa49845cd6fb7b7014ab06218b68fe702006`, inspected 2026-07-17 | LUKS2 TPM/recovery enrollment and PCR policy | Volume unlock is not tenant key management |
| KL-10 | Linux kernel 6.15, [Kernel Key Retention Service](https://docs.kernel.org/6.15/security/keys/core.html), accessed 2026-07-17 | Keyring permissions, expiry, revocation, scopes | Runtime facility, not hardware isolation or backup authority |
| KL-11 | IETF, [RFC 9846, TLS 1.3](https://www.rfc-editor.org/rfc/rfc9846), 2026; accessed 2026-07-17 | Authenticated confidential integrity-protected peer channel | Application must define identity and epoch semantics |
| KL-12 | IRTF CFRG, [RFC 8452, AES-GCM-SIV](https://www.rfc-editor.org/rfc/rfc8452), April 2019; accessed 2026-07-17 | Nonce-misuse-resistant AEAD alternative | Informational, not Standards Track or NIST-approved |

## Review of this review

The threat boundary, hierarchy, envelope binding, rotation, backup, crash, rank, compatibility, and deletion claims are explicit. Recommendations are not promoted as verified facts, and the report identifies current/mutable sources and machine-validation gaps. The decision is deliberately `DEFER`: accepting a concrete cryptographic profile without target inventory, an ADR, and destructive recovery evidence would exceed the available evidence.
