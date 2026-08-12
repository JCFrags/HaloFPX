# Minimum multi-user persistent-cache security profile

## Normative language

`HALOKV-MUST` and `HALOKV-SHOULD` are **local profile labels**, not IETF/NIST normative statements and not compliance claims. They define the proposed minimum implementation profile for PF-IR-07.

## Controls

| Control | Level | Requirement | Verification | Claim basis |
| --- | --- | --- | --- | --- |
| PFIR07-MP-001 | HALOKV-MUST | Use authenticated object encryption for every persistent private, project, or system/public cache object. | Format inspection and negative tag tests. | PFIR07-C050,C058 |
| PFIR07-MP-002 | HALOKV-MUST | Use an approved implementation of AES-256-GCM or ChaCha20-Poly1305 with a 128-bit authentication tag; AES-GCM-SIV may be an explicitly versioned alternative. | Cryptographic module inventory; test vectors. | PFIR07-C030-C036 |
| PFIR07-MP-003 | HALOKV-MUST | Use 96-bit nonces and enforce uniqueness within each AEAD key domain; never truncate or recycle nonce state. | Allocator review, collision tests, invocation metrics. | PFIR07-C031-C035 |
| PFIR07-MP-004 | HALOKV-MUST | Assign a separate rank key domain per epoch, or document an equivalently rigorous nonce partition. | KDF context and key-broker policy inspection. | PFIR07-C055 |
| PFIR07-MP-005 | HALOKV-MUST | Roll to a fresh epoch/key after uncertain nonce state, restored counter state, or suspected reuse. | Crash/restore fault-injection. | PFIR07-C033,C070 |
| PFIR07-MP-006 | HALOKV-MUST | Bind immutable authority, tenant, project, prefix, namespace, sharing class, rank, epoch, object type, format, codec, sizes, chunk position, CID, and manifest generation in canonical AAD as applicable. | Cross-field substitution tests. | PFIR07-C057 |
| PFIR07-MP-007 | HALOKV-MUST | Canonicalize AAD and identifier input with a versioned deterministic encoding and published test vectors. | Cross-language canonicalization suite. | PFIR07-C049,C057 |
| PFIR07-MP-008 | HALOKV-MUST | Authenticate before decompression, deserialization, semantic parsing, or data exposure. | Fuzzing and code-path review. | PFIR07-C059 |
| PFIR07-MP-009 | HALOKV-MUST | Normalize every unknown/corrupt/unauthenticated/wrong-key/wrong-principal/mismatch outcome to MISS_RECOMPUTE at the caller boundary. | Negative corpus and API contract tests. | PFIR07-C058,C063 |
| PFIR07-MP-010 | HALOKV-MUST | Never serve cached plaintext or partially parsed data after authentication failure. | Taint and exception-path review. | PFIR07-C059,C063 |
| PFIR07-MP-011 | HALOKV-MUST | Use purpose-separated encryption, identifier, manifest, and optional audit keys. | KDF/wrapping context review. | PFIR07-C054 |
| PFIR07-MP-012 | HALOKV-MUST | Use immutable authority-issued tenant/project identifiers; mutable names are metadata only. | Principal-schema and rename tests. | PFIR07-C027,C051 |
| PFIR07-MP-013 | HALOKV-MUST | Scope private deduplication/index lookup no wider than the authorized tenant/project sharing domain. | Index partition and authorization tests. | PFIR07-C052,C078 |
| PFIR07-MP-014 | HALOKV-MUST | Use a separate authority-owned, publication-controlled namespace for public/system prefixes. | Key ownership and write-ACL inspection. | PFIR07-C053 |
| PFIR07-MP-015 | HALOKV-MUST | Use a keyed, scope-specific private content identifier rather than a globally exposed raw plaintext digest. | CID derivation test vectors and index review. | PFIR07-C056 |
| PFIR07-MP-016 | HALOKV-MUST | Authenticate manifests and bind generation/epoch; enforce a trusted freshness policy where rollback matters. | Replay/rollback tests. | PFIR07-C060 |
| PFIR07-MP-017 | HALOKV-MUST | Write immutable objects with bounded lengths, fsync/atomic publish semantics, and publish the manifest/index last. | Power-failure and torn-write tests. | PFIR07-C059,C065 |
| PFIR07-MP-018 | HALOKV-MUST | Keep object and epoch keys out of cache-media plaintext; obtain them from a controlled KMS/HSM or wrapped-key broker. | Filesystem scan, deployment configuration, key-broker audit. | PFIR07-C040,C046 |
| PFIR07-MP-019 | HALOKV-MUST | Record key owner, custodian, authorization policy, lifecycle state, epoch, and destroy/escrow dependencies. | Key inventory and state-transition audit. | PFIR07-C040,C042 |
| PFIR07-MP-020 | HALOKV-MUST | Drain users and terminate/restart affected workers before declaring revocation complete; verify fscrypt key status and/or dm-crypt mapping closure when used. | Revocation runbook exercise. | PFIR07-C005-C007,C019-C020,C026 |
| PFIR07-MP-021 | HALOKV-MUST | Treat LUKS headers, header backups, detached headers, recovery credentials, KMS replicas, and snapshots as key-bearing dependencies. | Backup/key-copy inventory. | PFIR07-C014,C021,C042 |
| PFIR07-MP-022 | HALOKV-MUST | Restore only into a quarantine namespace and enforce current epoch floors, tombstones, format policy, and authentication before publication. | Restore drill. | PFIR07-C065 |
| PFIR07-MP-023 | HALOKV-MUST | Authorize and authenticate before export, then re-encrypt under the destination authority/scope and emit a new manifest. | Export integration tests and audit record. | PFIR07-C066 |
| PFIR07-MP-024 | HALOKV-MUST | Describe deletion as logical deletion unless a documented CE or media-sanitization procedure has met its preconditions and evidence requirements. | Deletion report wording and evidence review. | PFIR07-C041-C044,C067 |
| PFIR07-MP-025 | HALOKV-MUST | Do not claim per-object CE when an epoch key is shared by other live objects or usable key copies/backups remain. | Key-scope and copy inventory. | PFIR07-C068 |
| PFIR07-MP-026 | HALOKV-MUST | Do not treat discard/TRIM acknowledgement as secure deletion or physical sanitization evidence. | Configuration and deletion-report review. | PFIR07-C069,C083 |
| PFIR07-MP-027 | HALOKV-SHOULD | Use LUKS2/dm-crypt for the rank-local cache volume to protect offline media; keep discard off absent a reviewed exception. | Host storage configuration. | PFIR07-C039,C045,C069 |
| PFIR07-MP-028 | HALOKV-SHOULD | Use fscrypt v2 when directory-level offline isolation or selective unlock adds value; do not count it as object authenticity. | Policy version and key-removal drill. | PFIR07-C001-C009 |
| PFIR07-MP-029 | HALOKV-SHOULD | Avoid backing up regenerable cache content; when required, back up a coherent protected set and test quarantine restore. | Backup policy and restore evidence. | PFIR07-C064-C065 |
| PFIR07-MP-030 | HALOKV-MUST | Maintain rate limits, recomputation coalescing, quotas, and circuit breakers so fail-closed misses do not become an unbounded denial-of-service primitive. | Load/abuse tests. | PFIR07-C058,C063; RR25 |

## Reference deployment profile

```text
Host / rank
├── controlled boot and workload identity
├── LUKS2/dm-crypt encrypted cache volume (recommended)
│   ├── discard off by default
│   └── header backup policy explicit
├── filesystem
│   └── optional fscrypt v2 protected tree
└── HaloKV cache service
    ├── principal authorization before key/index lookup
    ├── rank-epoch K_enc / K_id / K_manifest
    ├── crash-safe nonce allocation
    ├── immutable AEAD object files
    ├── authenticated generation manifests
    ├── scoped private dedup index
    ├── separate public/system publisher namespace
    └── all validation failure => MISS_RECOMPUTE
```

## Required implementation gates

An implementation is not ready for persistent multi-user use until all of these gates pass:

1. Cross-tenant, cross-project, cross-prefix, cross-rank, cross-epoch, and wrong-suite substitution tests fail to miss.
2. Every malformed/truncated/oversized/duplicate-field envelope fails before data exposure.
3. AEAD negative tests use real modified ciphertext, tag, nonce, and AAD vectors.
4. Crash and snapshot tests prove no nonce/key domain resumes from uncertain allocator state.
5. Restore cannot publish directly into the live namespace and enforces tombstones/epoch floors.
6. Busy fscrypt files and busy dm-crypt mappings are exercised if those layers are used.
7. Logs and metrics contain no secret key material, plaintext, or externally distinguishable cryptographic error.
8. Backup/header/escrow dependencies appear in the key and deletion inventory.
9. Deletion reports correctly distinguish logical deletion, CE, and media sanitization.
10. Abuse/load tests show repeated invalid objects cannot cause unbounded memory allocation or recomputation amplification.

## Configuration floor

| Area | Floor |
|---|---|
| AEAD tag | 128 bits; no unauthenticated or shortened-tag compatibility path |
| Nonce | 96 bits, unique per key; state uncertainty creates new epoch |
| Encryption key | 256-bit suite key generated/derived through reviewed library/KMS |
| Private CID | Full HMAC-SHA-256 by default, scope-specific `K_id` |
| Canonical encoding | Versioned deterministic CBOR or equivalently reviewed deterministic encoding |
| Compression | Only after authentication on read; expansion and resource caps |
| Key delivery | Narrow rank-epoch/purpose keys; no parent root to cache worker |
| Key persistence | No plaintext object keys on cache media; protected wrapped records only |
| Freshness | Trusted epoch/generation floor for namespaces where rollback matters |
| Dedup | Private project/tenant scope; no default cross-tenant index |
| Public/system | Separate authority and publication workflow |
| Backup | None by default; quarantine restore if approved |
| Failure | One caller-visible miss/recompute class |
| Discard | Off by default; no deletion claim |

## Human approvals required before production

| ID | Decision | Why human | Required artifact | Safety default |
| --- | --- | --- | --- | --- |
| HD-01 | Principal authority | Determines which identity source can issue immutable tenant/project/service IDs and revoke them. | Authority policy, issuer trust roots, identifier lifecycle, audit ownership. | No cache-key issuance without authoritative immutable IDs. |
| HD-02 | Sharing and deduplication policy | Defines whether content equality may be shared within a project, tenant, explicit group, or public/system namespace. | Approved sharing classes and promotion workflow. | Private tenant/project scope; no cross-tenant dedup. |
| HD-03 | Key ownership and custody | Defines who can create, unwrap, escrow, rotate, revoke, and destroy roots and backups. | RACI/quorum, KMS policy, escrow and legal-hold rules. | Service cannot self-authorize parent-root access. |
| HD-04 | Backup requirement and retention | A regenerable cache may not merit backup; retaining it expands key copies and deletion obligations. | Data classification, RPO/RTO, retention and legal-hold policy. | No cache-content backup. |
| HD-05 | Freshness/rollback requirement | Some caches tolerate stale valid data; others need a monotonic source generation. | Maximum tolerated staleness and trusted state design. | Reject epochs/generations below current trusted floor. |
| HD-06 | Physical sanitization method | Depends on media, provider capability, reuse/disposition, regulatory policy, and evidence threshold. | Media-specific procedure, verification/validation criteria, disposition record. | Do not assert sanitization without procedure evidence. |
