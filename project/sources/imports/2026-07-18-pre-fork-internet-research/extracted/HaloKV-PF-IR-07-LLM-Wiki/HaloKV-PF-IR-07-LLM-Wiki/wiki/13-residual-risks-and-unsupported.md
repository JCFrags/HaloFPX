# Residual-risk ledger and unsupported claims

## Residual risks

Encryption narrows specific threat boundaries; it does not eliminate policy, implementation, online compromise, side-channel, backup, or media uncertainty. Each risk needs an accountable owner and an explicit treatment/acceptance decision.

| ID | Residual risk | Area | Impact | Likelihood | Treatment | Human decision | Basis |
| --- | --- | --- | --- | --- | --- | --- | --- |
| RR01 | Privileged live compromise can read keys or plaintext from an authorized process or kernel. | All options | High | Environment-dependent | Host isolation, least privilege, MAC, separate broker, rapid drain, incident response. | Accept host trust boundary or require stronger execution isolation. | PFIR07-C004,C075 |
| RR02 | Principal authority may map a user, service, tenant, or project incorrectly. | Tenant identity | High | Governance-dependent | Immutable authority IDs, signed authorization assertions, audit trail, separation of duties. | Select principal authority and lifecycle. | PFIR07-C027,C051 |
| RR03 | Sharing policy may authorize a dedup scope broader than users expect. | Dedup scope | High | Policy-dependent | Explicit sharing class in AAD/manifests; default private; review public promotion. | Approve tenant/project/public sharing policy. | PFIR07-C052,C053,C078 |
| RR04 | Parent key ownership, escrow, or recovery authority can expand access and defeat erasure. | Key hierarchy | High | Governance-dependent | Document owner/custodian, quorum, escrow, backup KEK, destroy authority, audit. | Assign key ownership and escrow policy. | PFIR07-C042,C054 |
| RR05 | Restored or rolled-back nonce allocator state can cause nonce reuse. | AEAD nonce | Critical | Implementation-dependent | Per-rank epoch; crash-safe reservation; restored state always receives new epoch/key. | Approve allocator and persistence implementation. | PFIR07-C031-C033,C055 |
| RR06 | Randomly generated nonces retain a non-zero collision probability and invocation bound. | AEAD nonce | High | Low under bounded use | 96-bit nonces from approved RNG, per-key invocation cap well below source limit, rotate early. | Select deterministic reservation or bounded random strategy. | PFIR07-C032,C035 |
| RR07 | AEAD accepts an old valid object unless trusted freshness is checked. | Replay | Medium/High | Threat-dependent | Epoch floor, manifest generation, expiration, source revision, monotonic publish record. | Define freshness and rollback tolerance. | PFIR07-C060,C076 |
| RR08 | Keyed identifiers reveal equality within their authorized dedup scope. | Identifiers/dedup | Medium | Expected | Minimize scope; rotate identifier epochs only with migration plan; avoid exposing index. | Accept within-scope equality leakage. | PFIR07-C056,C062,C077 |
| RR09 | Ciphertext length, object count, access pattern, allocation, and timing remain observable. | Privacy | Medium/High | Expected | Length buckets/padding where justified, scheduler isolation, per-tenant indexes, batching, rate limits. | Choose performance/privacy trade-off. | PFIR07-C015,C062 |
| RR10 | Shared cache timing can reveal a hit/miss or cross-tenant activity even when errors are normalized. | Timing leakage | Medium | Architecture-dependent | Tenant-local lookup path, equalized error classes, jitter/batching only if measured, quota isolation. | Set side-channel objective and performance budget. | PFIR07-C058,C062 |
| RR11 | fscrypt master-key removal leaves keys for open/in-use files. | fscrypt revocation | High | Expected under load | Drain processes, close files, change cwd/root references, verify removal status, restart if necessary. | Set revocation SLA that accounts for drain. | PFIR07-C005,C074 |
| RR12 | An active dm-crypt mapping retains the volume key while busy or deferred. | LUKS revocation | High | Expected under load | Stop I/O, unmount, close mapping, verify device-mapper state, avoid treating deferred close as completion. | Set maintenance and emergency availability policy. | PFIR07-C019,C020,C074 |
| RR13 | Keys/plaintext can remain in process memory, VFS caches, swap, crash dumps, registers, or hardware buffers. | Process retention | High | Environment-dependent | mlock where appropriate, no core dumps, encrypted swap, short-lived workers, explicit zeroization, process exit, dump controls. | Accept limits of software memory erasure. | PFIR07-C006,C007,C043 |
| RR14 | LUKS header backups or detached headers provide alternate unlock paths. | Backup/erasure | High | Operationally common | Inventory, encrypt separately, access-control, rotate/destroy coherently, test restoration under change control. | Retain or prohibit header backups. | PFIR07-C014,C021,C082 |
| RR15 | Backup KMS replicas, escrow, snapshots, and offline copies can outlive logical deletion. | Backup/deletion | High | Policy-dependent | Retention ledger, deletion propagation, legal holds, backup expiry, key-copy inventory. | Define retention and legal-hold policy. | PFIR07-C042,C064,C067 |
| RR16 | Restoring old ciphertext plus still-valid old keys can resurrect deleted or revoked content. | Restore | High | Process-dependent | Quarantine restore, trusted epoch floor, tombstone ledger, rewrap/re-encrypt into current context. | Define restore authorization and rollback window. | PFIR07-C060,C065 |
| RR17 | dm-integrity authenticated modes add performance, recovery, and maturity complexity. | Block integrity | Medium/High | Deployment-dependent | Prototype, benchmark, fault-inject, pin versions/configuration, keep object AEAD regardless. | Decide whether block integrity is worth operational cost. | PFIR07-C012,C016-C018 |
| RR18 | Discard pass-through leaks allocation and does not prove sanitization. | Discard | Medium | Expected when enabled | Default off; document exception; do not use as erasure evidence. | Approve performance exception. | PFIR07-C015,C069,C083 |
| RR19 | Sanitize commands may not reach remapped, failed, over-provisioned, or provider-controlled media. | Physical sanitization | High | Media/provider-dependent | Use media-specific procedure and authoritative vendor/provider capability; verify/validate; destroy where required. | Select purge/destroy method and acceptance evidence. | PFIR07-C041-C044,C083 |
| RR20 | A cloud or storage provider may not expose evidence sufficient to validate media sanitization. | Hosted media | High | Provider-dependent | Contractual controls, provider attestations/evidence, encryption key control, data-lifecycle audit. | Accept provider assurance model or avoid persistent cache there. | PFIR07-C044 |
| RR21 | Different implementations may canonicalize AAD, identifiers, or manifests differently. | Interoperability | High | Implementation-dependent | Versioned deterministic encoding, test vectors, reject duplicate/unknown critical fields, cross-language conformance. | Approve canonical format and compatibility policy. | PFIR07-C049,C057 |
| RR22 | Quarantine storage can become an alternate serving path or retain sensitive evidence indefinitely. | Quarantine | Medium/High | Process-dependent | Separate credentials/path, no cache lookup integration, retention cap, access logging, incident hold workflow. | Set evidence retention and access authority. | PFIR07-C063,C065,C070 |
| RR23 | Detailed cryptographic error logs can become a tenant oracle or leak key/identifier material. | Telemetry | Medium | Implementation-dependent | Opaque external miss; access-controlled internal reason code; redact secrets; aggregate metrics; rate limit. | Set logging retention and analyst access. | PFIR07-C058,C062 |
| RR24 | A compromised public/system publisher can distribute semantically malicious but cryptographically valid content. | Public/system prefix | High | Authority-dependent | Two-person publication, reproducible generation, signed immutable manifests, rollback and emergency revoke. | Select publisher authority and review process. | PFIR07-C053 |
| RR25 | Strict fail-closed behavior can amplify denial-of-service through repeated misses and recomputation. | Availability | Medium/High | Threat/load-dependent | Per-principal quotas, backoff, duplicate recompute suppression, circuit breakers, protected authoritative source. | Set availability and abuse-control budget. | PFIR07-C058,C063 |

The machine-readable ledger is [`matrices/residual-risks.csv`](../matrices/residual-risks.csv).

## Explicitly unsupported or rejected claims

The following labels are literal claim-registry entries. They are included to prevent lower-layer features, primitive names, or deletion commands from being converted into unsupported assurance language.


[CLAIM:PFIR07-C071][CLASS:UNSUPPORTED][STATUS:REJECTED][SRC:C003,C009]

**Unsupported claim: fscrypt alone authenticates HaloKV objects or detects all offline tampering.**  
False for selected boundary.

[CLAIM:PFIR07-C072][CLASS:UNSUPPORTED][STATUS:REJECTED][SRC:C011-C012,C039]

**Unsupported claim: ordinary LUKS2/dm-crypt XTS provides per-object authenticity or tenant binding.**  
Confidentiality boundary only.

[CLAIM:PFIR07-C073][CLASS:UNSUPPORTED][STATUS:REJECTED][SRC:C041-C044]

**Unsupported claim: unlink, overwrite, TRIM, discard, or filesystem free-space release is automatically secure deletion.**  
Media-dependent and unverified.

[CLAIM:PFIR07-C074][CLASS:UNSUPPORTED][STATUS:REJECTED][SRC:C005-C007,C019-C020,C026,C029]

**Unsupported claim: key revocation instantly removes access from open files, active mappings, kernel keyrings, or processes.**  
Drain and memory lifecycle required.

[CLAIM:PFIR07-C075][CLASS:UNSUPPORTED][STATUS:REJECTED][SRC:C004,C045]

**Unsupported claim: at-rest encryption protects plaintext from a compromised root account, kernel, hypervisor, DMA-capable actor, or authorized live process.**  
Outside selected control boundary.

[CLAIM:PFIR07-C076][CLASS:UNSUPPORTED][STATUS:REJECTED][SRC:C060]

**Unsupported claim: AEAD authentication by itself prevents rollback or replay of an older valid object.**  
Freshness state is separate.

[CLAIM:PFIR07-C077][CLASS:UNSUPPORTED][STATUS:REJECTED][SRC:C056,C062]

**Unsupported claim: keyed content identifiers eliminate access-pattern, size, timing, or within-scope equality leakage.**  
They reduce public digest exposure only.

[CLAIM:PFIR07-C078][CLASS:UNSUPPORTED][STATUS:REJECTED][SRC:C052,C062]

**Unsupported claim: cross-tenant global deduplication is privacy-neutral merely because payloads are encrypted.**  
Lookup and timing can form confirmation oracles.

[CLAIM:PFIR07-C079][CLASS:UNSUPPORTED][STATUS:REJECTED][SRC:C041-C044]

**Unsupported claim: cryptographic erase is physical zeroization or proves all remanent media states are unrecoverable.**  
CE is key inaccessibility under stated assumptions.

[CLAIM:PFIR07-C080][CLASS:UNSUPPORTED][STATUS:REJECTED][SRC:C024,C027]

**Unsupported claim: TPM-, FIDO2-, or PKCS#11-assisted volume unlock establishes HaloKV tenant/project principal authority.**  
It establishes a device/credential unlock condition.

[CLAIM:PFIR07-C081][CLASS:UNSUPPORTED][STATUS:REJECTED][SRC:C030-C049]

**Unsupported claim: selecting a NIST-specified primitive automatically makes the HaloKV implementation FIPS validated, certified, or compliant.**  
Implementation/module/program status is separate.

[CLAIM:PFIR07-C082][CLASS:UNSUPPORTED][STATUS:REJECTED][SRC:C014,C021-C022,C042]

**Unsupported claim: removing a LUKS keyslot is irreversible while header backups, alternate keyslots, escrow, or snapshots exist.**  
All unlock paths matter.

[CLAIM:PFIR07-C083][CLASS:UNSUPPORTED][STATUS:REJECTED][SRC:C015,C041-C044]

**Unsupported claim: an SSD or storage-provider discard acknowledgement proves physical cells have been sanitized.**  
Use media/provider-specific sanitize evidence and validation.


## Compliance and validation boundary

This bundle compares technical controls and source-defined behavior. It does not determine whether a deployment is FIPS validated, Common Criteria evaluated, certified under any regulatory program, contractually compliant, or securely deleted. Those conclusions require the exact cryptographic module, operational environment, configuration, policy, evidence, and applicable program requirements.

[CLAIM:PFIR07-C081][CLASS:UNSUPPORTED][STATUS:REJECTED][SRC:C030-C049]

## Decision-owner checklist

Before closing `OPEN-SCOPE-01`, humans must sign off on:

* authoritative identity issuer and immutable ID lifecycle;
* private/project/shared/public dedup scopes;
* system-prefix publisher and rollback authority;
* root/tenant/project key owner, custodian, escrow, and destroy quorum;
* accepted live-host trust boundary;
* nonce allocator design and local invocation ceilings;
* rollback/staleness tolerance;
* cache backup RPO/RTO and retention exception, if any;
* quarantine and incident evidence retention;
* CE scope, media-sanitization method, and acceptable evidence;
* performance/privacy trade-offs for padding, indexing, discard, and isolation.
