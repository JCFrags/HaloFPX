# Threat model and invariant

## System model

HaloKV is treated as a **non-authoritative, rank-local persistent cache**. Correctness comes from recomputation or retrieval through an authoritative pipeline. Persistent entries may include object chunks, manifests, indexes, metadata, and optional public/system-prefix artifacts. A rank can be restarted, rebuilt, or cold-started without treating cached bytes as authoritative state.

## Assets

1. Private payload confidentiality and object semantics.
2. Tenant/project authorization boundaries and immutable principal identifiers.
3. Integrity of object bytes, format fields, scoped content identifiers, and manifests.
4. Key material, nonce allocation state, epoch floors, tombstones, and publication keys.
5. Availability of the authoritative recomputation path and controlled cache-hit behavior.
6. Evidence needed for key lifecycle, backup/restore, deletion, quarantine, and incident response.

## Adversary capabilities considered

* Reads or modifies an offline cache filesystem or block device.
* Copies objects across paths, ranks, tenants, projects, prefixes, epochs, or backups.
* Supplies malformed lengths, headers, nonces, tags, manifests, or compressed payloads.
* Replays an older valid object or restores an older snapshot.
* Observes sizes, allocation, timing, and hit/miss-dependent work at boundaries available to it.
* Obtains one tenant/project/rank/epoch key and attempts lateral use.
* Interrupts writes, reencryption, restore, deletion, or key rotation.
* Retains backups, snapshots, header copies, escrow, or process-held keys after revocation.

## Trust boundaries

| Boundary | Trusted for | Not trusted for |
|---|---|---|
| Principal authority | Immutable tenant/project/service identity and authorization assertions | Cache-byte integrity or key erasure evidence |
| Key authority / KMS | Root custody, unwrap authorization, state/audit | Correct object AAD supplied by a compromised cache process |
| Rank cache process | Requested cryptographic operation while authorized | Long-term root custody or self-issuing principal identity |
| Host kernel / filesystem | Correct execution in the at-rest model | Resistance after privileged live compromise |
| Cache media / backup store | Availability of stored ciphertext | Confidentiality, authenticity, freshness, or deletion evidence |
| Public/system publisher | Authorized publication under its prefix | Tenant-private promotion or semantic safety without review |
| Authoritative recomputation path | Produces the source result used on miss | Cache storage is never allowed to override it after validation failure |

## Out of cryptographic scope

The selected controls do not claim to protect plaintext from a compromised live kernel, root account, hypervisor, authorized process, debugger, DMA-capable actor, malicious authoritative computation, or compromised parent key authority. Network transport security, hardware side channels, endpoint export controls, legal retention, and service-level denial-of-service controls require separate designs.

[CLAIM:PFIR07-C075][CLASS:UNSUPPORTED][STATUS:REJECTED][SRC:C004,C045]

## Mandatory invariant

> **Every cache read is either a fully authenticated, policy-consistent hit or a miss/recompute. There is no third state exposed to the caller.**

The implementation may record an internal reason code such as `UNKNOWN_SUITE`, `KEY_UNAVAILABLE`, `AAD_MISMATCH`, `TAG_FAIL`, `CID_FAIL`, `STALE_GENERATION`, or `FORMAT_CORRUPT`, but the caller receives only `MISS_RECOMPUTE`. The internal channel is access-controlled, redacted, rate-limited, and unsuitable for tenant-controlled branching.

[CLAIM:PFIR07-C058][CLASS:SYNTHESIS][STATUS:REQUIRED][SRC:C003,C016,C030,C034]

[CLAIM:PFIR07-C063][CLASS:SYNTHESIS][STATUS:REQUIRED][SRC:C030,C034,C058-C059]

## Security objectives

| Objective | Required behavior |
|---|---|
| Confidentiality | Persistent payloads are never written in plaintext; lower-layer encryption may add stolen-media protection. |
| Authenticity | Object tag covers ciphertext and canonical AAD; manifest authentication covers membership and generation. |
| Principal binding | Immutable authority/tenant/project/prefix/namespace/sharing/rank values are cryptographically bound and independently authorized. |
| Domain separation | Different purposes, ranks, epochs, and sharing scopes do not reuse keys. |
| Nonce safety | Unique nonce per AEAD key; uncertain state creates a new key epoch. |
| Replay policy | Older valid epochs/generations are rejected according to trusted freshness rules. |
| Fail closed | Authentication and policy checks precede decompression, parsing, and exposure. |
| Recoverability | Corrupt or unavailable cache content is discarded/quarantined and recomputed; cache availability never weakens validation. |
| Erasure honesty | Reports distinguish logical deletion, key inaccessibility/CE, and media sanitization. |

## Threat-to-control matrix

The complete machine-readable matrix is in [`matrices/threat-to-control.csv`](../matrices/threat-to-control.csv). The highest-impact rows are:

| ID | Threat | Object-AEAD effect | Operational control | Residual |
| --- | --- | --- | --- | --- |
| T01 | Stolen or decommissioned rank-local drive, mapping closed | Mitigates payload disclosure if object keys absent | KMS separation; no plaintext keys on cache media; media inventory | Visible metadata, sizes, headers, backups, keys elsewhere, weak unlock credentials |
| T02 | Offline modification or ciphertext substitution | Detects payload/AAD substitution; wrong tag => miss | Immutable objects; manifest authentication; quarantine evidence | Replay of an older valid object without trusted freshness state |
| T03 | Online co-tenant reads another tenant cache entry | Per-scope key and AAD principal binding deny wrong-tenant authentication | Authoritative principal mapping; per-request authorization; service isolation | Compromised cache service, kernel, or authority mapping |
| T04 | Compromised root/kernel/hypervisor while cache is live | Keys/plaintext accessible to compromised trusted computing base | Host hardening, measured boot, MAC, least privilege, separate key broker, rapid drain | Cryptography cannot make live plaintext unavailable to an authorized computation path |
| T05 | Object copied between ranks or peers | Rank and epoch in KDF/AAD make copied object fail authentication | Rank identity assigned by authority; peer protocol mutually authenticated | Incorrect authority assignment or shared rank key |
| T06 | Wrong key epoch, tenant, project, prefix, codec, or manifest | AAD/key mismatch causes AEAD FAIL; normalize to miss | Single miss path; rate-limited structured diagnostics | Diagnostic side channels if error classes leak to callers |
| T07 | AEAD nonce reuse after crash or state rollback | Can catastrophically weaken GCM/ChaCha20-Poly1305 | Per-rank epoch keys; crash-safe allocator; epoch rollover on uncertainty; invocation ceilings | Implementation error or restored counter snapshot without epoch change |
| T08 | Replay of an old but valid object or manifest | Authenticates replayed object as valid unless freshness is checked | Trusted epoch floor, generation, expiration, source version, monotonic manifest state | Rollback of the trusted freshness store itself |
