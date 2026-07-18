# Content addressing, deduplication, identifiers, and manifests

## Why a raw global digest is unsafe for private cache content

A public unkeyed digest of canonical plaintext allows any observer who can query or inspect the index to test guesses and learn equality across tenants. Encryption of the payload does not remove that lookup oracle. Cross-tenant hit latency, admission behavior, storage accounting, or error detail can further confirm the guess.

The private identifier is therefore scoped and keyed:

```text
semantic_bytes = deterministic_encode(object_type, schema, codec-normalized-content)
scoped_cid = HMAC-SHA-256(K_id, "HaloKV/CID/v1" || semantic_bytes)
```

Use the complete 256-bit result internally unless a collision analysis and protocol version explicitly approve truncation. The identifier key is separate from `K_enc` and `K_manifest`. The `sharing_class` and namespace are fixed by the key domain and also bound in object/manifest AAD.

[CLAIM:PFIR07-C056][CLASS:SYNTHESIS][STATUS:RECOMMENDED][SRC:C047,C052]

## Deduplication scopes

| Sharing class | CID/index key | Maximum lookup scope | Write authority |
|---|---|---|---|
| `private-project` | Project/namespace `K_id` | One project | Authorized project workloads/cache service |
| `private-tenant` | Tenant/namespace `K_id` | One tenant | Authorized tenant workloads/cache service |
| `explicit-group:<id>` | Group authority `K_id` | Current authorized group | Group-controlled publisher/cache service |
| `public-system` | System publisher identifier/signature policy | Explicit public/system namespace | Publisher only; tenants read only |

The safer default is `private-project` when project isolation exists, otherwise `private-tenant`. Cross-tenant private dedup is rejected. A public/system object is not created by observing that several tenants produced equal private content; it enters the public namespace only through an explicit publication workflow.

[CLAIM:PFIR07-C052][CLASS:SYNTHESIS][STATUS:RECOMMENDED][SRC:C047,C056,C061-C062]

[CLAIM:PFIR07-C053][CLASS:SYNTHESIS][STATUS:RECOMMENDED][SRC:C027,C052]

## Ciphertext and dedup

Randomized AEAD intentionally makes repeated encryption under distinct nonces produce different ciphertext. The logical dedup process is:

1. Authorize the request for a sharing scope.
2. Compute the scope-specific keyed CID over canonical semantics.
3. Lookup only within that scope's index.
4. On a candidate hit, authenticate the object and validate AAD, CID, and freshness.
5. On any failure, treat it as a miss; quarantine/delete candidate and recompute.
6. On a miss, encrypt a new object with a fresh nonce and publish atomically.

Do not deduplicate private objects by raw plaintext digest, storage-block equality, unauthenticated compressed bytes, or ciphertext equality.

[CLAIM:PFIR07-C061][CLASS:SYNTHESIS][STATUS:SUPPORTED][SRC:C030-C036]

## Canonical semantic input

The CID may be over exact plaintext bytes or normalized semantics. The choice must be versioned and deterministic. If normalization is used, it must define:

* byte encoding and map-key order;
* numeric and Unicode representation;
* treatment of default/absent/null fields;
* codec and compression boundaries;
* stable object type/schema version;
* chunking algorithm/version;
* whether volatile timestamps or request IDs are excluded;
* duplicate-key rejection and extension handling.

Two implementations that disagree must generate different namespace/schema versions rather than silently sharing identifiers.

[CLAIM:PFIR07-C049][CLASS:SOURCE][STATUS:SUPPORTED][SRC:RFC8949 §4.2]

## Manifests

A manifest authenticates object membership, order, chunk lengths, scoped CIDs, key epochs, source revision, sharing class, and generation. It is either:

* AEAD-encrypted/MACed under `K_manifest` when only scope members need to read it; or
* deterministically encoded and digitally signed by a separate public/system publisher when broad verification without shared secret is required.

Manifest identity is not a substitute for authenticating each referenced object. Each object binds its manifest ID/generation and chunk position in AAD; the manifest binds the object key/CID/length tuple. This bidirectional binding prevents path-only substitution.

## Privacy-preserving identifiers are limited controls

Keyed CIDs reduce public dictionary and cross-scope equality exposure. They do not hide:

* equality within the authorized scope;
* object size, count, or access frequency;
* which tenant/rank performed work when observable elsewhere;
* hit/miss timing unless the architecture isolates it;
* semantic leakage from public/system artifacts;
* leakage after `K_id` compromise.

[CLAIM:PFIR07-C062][CLASS:SYNTHESIS][STATUS:RESIDUAL-RISK][SRC:C002,C011,C015,C056-C061]

[CLAIM:PFIR07-C077][CLASS:UNSUPPORTED][STATUS:REJECTED][SRC:C056,C062]

## Cross-tenant timing leakage controls

Cryptography alone is insufficient. Use separate per-tenant/project indexes, authorization before lookup, quota/accounting after authorization, bounded constant-shaped miss errors, recomputation coalescing only inside the same scope, and scheduler/resource isolation for sensitive tenants. Padding or artificial delay should be introduced only against a measured threat because it can worsen denial-of-service and rarely makes full execution time constant.

## Public/system-prefix poisoning controls

A public namespace needs a publisher-specific manifest signature or MAC authority, immutable release/generation, reproducible generation where feasible, review, emergency revoke, and rollback protection. A valid signature proves publisher authorization—not semantic correctness—so publication review and source provenance remain required.
