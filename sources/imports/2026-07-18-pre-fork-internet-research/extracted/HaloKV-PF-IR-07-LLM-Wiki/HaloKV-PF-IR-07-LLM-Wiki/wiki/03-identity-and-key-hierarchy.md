# Tenant identity, principal binding, and key hierarchy

## Identity is an authority decision

Cryptographic keys do not decide who a tenant is. HaloKV accepts only immutable, authority-issued identifiers and an authorization assertion that binds the requesting workload to tenant/project/prefix scope. Human-readable names, Unix usernames, paths, container names, ranks assigned by an untrusted peer, and cloud display labels are metadata—not key-derivation authority.

Required identity inputs:

| Field | Requirement |
|---|---|
| `authority_id` | Stable identifier for the identity/key policy authority. |
| `principal_id` | Stable workload/user/service principal identifier used for authorization audit. |
| `tenant_id` | Immutable tenant UUID; mandatory for private namespaces. |
| `project_id` | Immutable project UUID or explicit null sentinel; never omitted ambiguously. |
| `prefix_id` | Private prefix or separately governed public/system prefix. |
| `namespace_id` | Cache format/application namespace. |
| `sharing_class` | `private-tenant`, `private-project`, `explicit-group:<id>`, or `public-system`. |
| `rank_id` | Authority-assigned rank/peer identity, unique within its namespace and lifecycle. |

TPM2, FIDO2, PKCS#11, a boot passphrase, or possession of an fscrypt key may establish an unlock condition. They do not, without a separate authority mapping, prove the HaloKV tenant/project principal.

[CLAIM:PFIR07-C027][CLASS:INFERENCE][STATUS:SUPPORTED][SRC:SYSTEMD-CRYPTTAB-261.1 TPM2 option; principal model]

[CLAIM:PFIR07-C080][CLASS:UNSUPPORTED][STATUS:REJECTED][SRC:C024,C027]

## Key hierarchy

| Level | Domain | Identifier | Key material | Binding | Rotation scope | Notes |
| --- | --- | --- | --- | --- | --- | --- |
| 0 | Authority root | authority_id | Root KEK or root derivation key in KMS/HSM | Organization/security-domain authority | All subordinate domains; avoid direct data encryption | Human decision: owner, custody, quorum, escrow. |
| 1 | Tenant root | tenant_id | Random tenant root wrapped by authority root, or KDF-isolated tenant branch | Immutable tenant UUID from principal authority | All tenant projects/prefixes | Do not derive from tenant display name. |
| 2 | Project root | project_id | Random/wrapped project root or context-derived branch | Immutable project UUID and authorization relation | Project cache | Optional if tenant has no project separation; do not omit field silently. |
| 3 | Prefix authority | prefix_id | Prefix branch | Private prefix, public/system prefix, or service-controlled prefix | Objects under prefix | System/public prefix uses a separate authority, not a tenant key. |
| 4 | Namespace and sharing class | namespace_id + sharing_class | Namespace branch | private-tenant, private-project, public-system, explicit-share-group | Deduplication and authorization domain | This level fixes maximum CID/dedup scope. |
| 5 | Rank/peer branch | rank_id | Rank root delivered/wrapped to one rank | Authority-assigned rank identity | One rank within one namespace | Prevents cross-rank ciphertext reuse and partitions nonce state. |
| 6 | Epoch | epoch_id | Random epoch root or derived epoch branch | Monotonic or random unique epoch identifier | New writes for rank/namespace | Old epoch can be read-only during bounded migration; uncertain nonce state requires new epoch. |
| 7 | Purpose keys | purpose | K_enc, K_id, K_manifest, optional K_audit; optionally K_wrap | Literal versioned purpose labels | One cryptographic purpose | Never reuse AEAD key as HMAC/CID/manifest key. |
| 8 | Optional per-object DEK | object_id or random DEK id | Random DEK wrapped by K_wrap | One object or independently erasable group | Object/group | Needed only for fine-grained CE; increases metadata and key-record cost. |

## Derivation/wrapping construction

Two acceptable implementation families are:

1. **Random child roots wrapped by parent KEKs.** The KMS generates a random tenant/project/rank/epoch root and stores only a parent-wrapped blob plus policy metadata. This supports explicit destruction records and flexible reparenting.
2. **Reviewed KDF branches.** A root derivation key applies HKDF or an SP 800-108 construction with canonical versioned `Label` and `Context`. The context contains all hierarchy identifiers and cannot be influenced without authorization.

A hybrid is common: random authority/tenant/project/epoch roots are wrapped in KMS, while short-lived rank/purpose keys are derived. Never derive keys directly from tenant names, passwords, public digests, paths, timestamps, or low-entropy rank numbers. Human credentials, if unavoidable for lower-layer unlock, use the selected platform KDF and parameters; they do not replace random data keys.

Example context, encoded deterministically rather than concatenated ad hoc:

```text
label   = "HaloKV/PFIR07/v1/purpose-key"
context = encode({
  authority_id, tenant_id, project_id, prefix_id,
  namespace_id, sharing_class, rank_id, epoch_id,
  purpose, algorithm_suite, output_length
})
```

[CLAIM:PFIR07-C038][CLASS:SOURCE][STATUS:SUPPORTED][SRC:NIST-SP800-108R1U1 §§4-5]

[CLAIM:PFIR07-C046][CLASS:SOURCE][STATUS:SUPPORTED][SRC:NIST-SP800-133R2]

## Purpose separation

At minimum:

* `K_enc`: object AEAD only.
* `K_id`: private scoped content identifier only.
* `K_manifest`: manifest MAC/AEAD or manifest signing-key wrap only.
* `K_audit`: optional pseudonymous audit correlation, never object encryption.
* `K_wrap`: optional per-object DEK wrapping key where independently scoped CE is required.

Do not reuse the same bytes across these purposes, even if the underlying primitive accepts the key length. Include literal purpose and suite identifiers in the KDF/wrapping record.

[CLAIM:PFIR07-C054][CLASS:SYNTHESIS][STATUS:RECOMMENDED][SRC:C037-C040,C046]

## Key records and authority checks

Every key or wrapped-root record should contain:

```text
key_id, key_type, parent_key_id, authority_id, owner_id, custodian_policy,
principal_scope, tenant_id, project_id, prefix_id, namespace_id, sharing_class,
rank_id, epoch_id, purpose, suite, state, created_at, activates_at,
deactivates_at, compromise_time, destroy_time, escrow_dependencies,
backup_dependencies, wrapped_key_blob, wrapping_key_version, policy_hash
```

Before unwrapping or deriving, the broker verifies the caller identity, requested operation, exact context, key lifecycle state, and current revocation/epoch floor. A cache process cannot request a broader parent key and derive arbitrary tenants locally.

## Lifecycle states

Map local states to an explicit lifecycle: `PREACTIVE → ACTIVE → READ_ONLY/DEACTIVATED → COMPROMISED or DESTROY_PENDING → DESTROYED`. `READ_ONLY` is a HaloKV operational substate: old epoch decrypt may be allowed during bounded migration, while all writes use the new epoch. A compromised epoch is never returned for serving reads unless an incident evidence workflow explicitly reads ciphertext in quarantine without trusting it.

[CLAIM:PFIR07-C040][CLASS:SOURCE][STATUS:SUPPORTED][SRC:NIST-SP800-57P1R5 §7]

## Rotation scopes

* Credential rotation: replace passphrase/token/keyslot wrapper; payload key may be unchanged.
* Wrapping-key rotation: rewrap child roots without changing object ciphertext, when the hierarchy allows it.
* Epoch rotation: all new object writes use a fresh epoch AEAD key and nonce state.
* Data-key rotation: re-encrypt or recompute object payloads under a new `K_enc`/DEK.
* Authority-root rotation: controlled rewrap/reissue across all descendants; highest-risk operation.

The runbook defaults to epoch rollover plus lazy cache recomputation, not bulk in-place transformation.
