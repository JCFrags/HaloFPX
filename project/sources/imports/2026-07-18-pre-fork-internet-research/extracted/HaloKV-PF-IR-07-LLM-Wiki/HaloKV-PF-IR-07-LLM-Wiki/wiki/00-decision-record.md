# PF-IR-07 decision record

**Status:** Proposed standards-backed option set for `OPEN-SCOPE-01`  
**Priority:** P1  
**Research access date:** 2026-07-18  
**Bundle version:** 1.0.0

## Decision

HaloKV's minimum multi-user persistent-cache profile uses **application/object-level authenticated encryption for every persisted object and manifest**. It may add **LUKS2/dm-crypt** for rank-local volume confidentiality and **fscrypt v2** for selective directory/key-domain isolation, but neither lower layer substitutes for object authentication or tenant binding.

[CLAIM:PFIR07-C050][CLASS:SYNTHESIS][STATUS:RECOMMENDED][SRC:C002-C004,C009,C030-C035,C045]

The object layer binds immutable authority, tenant, project, prefix, namespace, sharing class, rank, epoch, object format, codec, chunk position, scoped content identifier, and manifest generation in canonical associated data. Any unknown, corrupt, malformed, unauthenticated, wrong-key, wrong-principal, wrong-rank, stale, or mismatched object returns the single caller-visible result `MISS_RECOMPUTE` and is recomputed from an authoritative source.

[CLAIM:PFIR07-C058][CLASS:SYNTHESIS][STATUS:REQUIRED][SRC:C003,C016,C030,C034]

## Selected option set

| Layer | Minimum status | Purpose | Explicit non-claim |
|---|---|---|---|
| Object AEAD | **Required** | Authenticated tenant/object semantics, failure normalization, scoped identifiers, replay metadata | Does not protect against a compromised live process/kernel and does not by itself stop replay. |
| LUKS2/dm-crypt | **Recommended host-volume baseline** where operationally supported | Stolen-media and offline block-volume confidentiality; boot-time unlock | Ordinary XTS mode is not object authenticity, tenant isolation, or secure deletion. |
| fscrypt v2 | **Conditional defense in depth** | Selective tree unlock and filesystem-level key domains | Not authenticated object storage; busy files can retain keys after master-key removal. |
| dm-integrity + authenticated dm-crypt | **Optional, evaluate** | Block-level integrity/replay-related storage features depending configuration | Selected stack documents authenticated disk encryption as experimental; object AEAD remains mandatory. |

## Deduplication policy

* Private data: keyed content identifiers and dedup indexes are scoped no wider than the explicitly authorized tenant/project sharing domain.
* Shared groups: use a distinct sharing-class authority and key domain; membership changes trigger a new epoch and do not retroactively erase copies already obtained.
* Public/system prefixes: use a separate publisher authority, publication keys, immutable signed/authenticated manifests, rollback policy, and read-only tenant access. Never merge private tenant objects into a global pool by digest equality.

[CLAIM:PFIR07-C052][CLASS:SYNTHESIS][STATUS:RECOMMENDED][SRC:C047,C056,C061-C062]

[CLAIM:PFIR07-C053][CLASS:SYNTHESIS][STATUS:RECOMMENDED][SRC:C027,C052]

## Key lifecycle policy

Use a versioned hierarchy: authority → tenant → project → prefix → namespace/sharing class → rank → epoch → purpose. Encryption, private identifier, manifest, and audit keys are distinct. A suspected key compromise, nonce-state uncertainty, or restored allocator snapshot creates a new epoch; it does not resume the old nonce/key domain.

[CLAIM:PFIR07-C054][CLASS:SYNTHESIS][STATUS:RECOMMENDED][SRC:C037-C040,C046]

[CLAIM:PFIR07-C055][CLASS:SYNTHESIS][STATUS:RECOMMENDED][SRC:C031-C033,C035-C036]

## Backup and deletion posture

The cache is recomputable, so the default is **no backup of cache content**. A business-approved exception backs up a coherent set of ciphertext, authenticated manifests, format metadata, wrapped epoch roots, freshness floors/tombstones, and any lower-layer key/header dependencies. Restore goes to quarantine, never directly to the serving path.

Logical deletion removes authorization, indexes, manifests, serving references, and scheduled objects. It is not labeled sanitization. Cryptographic erasure is claimed only for a precisely stated key scope after all usable key copies, wrappers, backups, escrow, snapshots, open-file/mapping dependencies, and required evidence have been addressed. Physical media retirement follows a media-specific sanitization procedure with verification and validation; discard is not treated as proof.

[CLAIM:PFIR07-C064][CLASS:SYNTHESIS][STATUS:RECOMMENDED][SRC:C013-C014,C040-C044]

[CLAIM:PFIR07-C067][CLASS:SYNTHESIS][STATUS:REQUIRED][SRC:C041-C044]

[CLAIM:PFIR07-C068][CLASS:SYNTHESIS][STATUS:CONDITIONAL][SRC:C042-C044,C054]

## Human decisions left open

The technical profile intentionally does not choose the principal authority, sharing-policy authority, key owner/custodian, backup retention, rollback tolerance, or physical-media disposition authority. Those choices must be made by accountable humans and reflected in authorization and KMS policies. See [Human decisions](../matrices/human-decisions.csv).

## Decision consequences

**Positive:** object substitution and wrong-domain reuse fail closed; private dedup scope is explicit; rank/epoch separation limits nonce-state and compromise blast radius; lower storage layers remain available for offline defense.

**Cost:** canonical format governance, key-broker integration, nonce-state durability, negative-path testing, recomputation load, restore quarantine, and a non-trivial operational key inventory.

**Residual:** traffic analysis, timing, object size/count, within-scope equality, live privileged compromise, authority mistakes, and unknown key copies are not eliminated. They are tracked in [the residual-risk ledger](13-residual-risks-and-unsupported.html).
