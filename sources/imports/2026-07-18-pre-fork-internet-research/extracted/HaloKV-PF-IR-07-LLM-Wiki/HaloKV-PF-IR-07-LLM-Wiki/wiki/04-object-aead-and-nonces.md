# Object AEAD, nonce allocation, and associated data

## Algorithm profile

The minimum profile permits two primary suites, each with a 128-bit authentication tag:

| Suite ID | Algorithm | Nonce | Local policy |
|---|---|---|---|
| `HKV1-A256GCM` | AES-256-GCM | 96 bits | Unique per `K_enc`; deterministic reservation preferred; full tag; rotate well before implementation/source limits. |
| `HKV1-C20P` | ChaCha20-Poly1305 (IETF) | 96 bits | Unique per `K_enc`; full tag; use high-quality implementation with test vectors. |
| `HKV1-A256GCMSIV` | AES-256-GCM-SIV | 96 bits | Optional, separately negotiated/versioned; misuse resistance is defense in depth, not permission for intentional reuse. |

Primitive selection does not imply implementation validation, FIPS status, or compliance. Use a reviewed platform cryptographic library, disable unauthenticated modes, and include the exact suite in the authenticated format.

[CLAIM:PFIR07-C030][CLASS:SOURCE][STATUS:SUPPORTED][SRC:NIST-SP800-38D §§5.2,7]

[CLAIM:PFIR07-C031][CLASS:SOURCE][STATUS:SUPPORTED][SRC:NIST-SP800-38D §8]

[CLAIM:PFIR07-C036][CLASS:SOURCE][STATUS:SUPPORTED][SRC:RFC8452 §§3,9]

## Envelope

The persisted record consists of a small bounded header, canonical AAD, ciphertext, and tag. The header fields required to locate a key and bound allocation are themselves repeated or committed in the AAD. No untrusted length may cause an allocation above configured limits.

```text
magic | envelope_version | suite_id | flags | key_ref | nonce |
aad_length | ciphertext_length | aad | ciphertext | tag
```

The CDDL model is in [`schemas/object-envelope.cddl`](../schemas/object-envelope.cddl). It is a design schema, not an IANA or standards-body format.

## Canonical associated data

The AAD map is deterministic and versioned. Mandatory or conditionally mandatory fields:

```text
aad_version
algorithm_suite
authority_id
principal_scope_id
tenant_id
project_id_or_null
prefix_id
namespace_id
sharing_class
rank_id
epoch_id
key_id
object_type
object_schema_version
codec_id
compression_id
uncompressed_length
ciphertext_length
chunk_index
chunk_count
scoped_content_id
manifest_id
manifest_generation
source_revision_or_null
created_time_bucket_or_null
critical_extensions
```

AAD is authenticated but not encrypted; do not place secrets in it. Where identifiers themselves are sensitive, use pseudonymous stable IDs assigned by the authority, not plaintext names. Reject unknown critical extensions and duplicate map keys. Include lengths in AAD and enforce local maximums before attempting decryption or decompression.

[CLAIM:PFIR07-C057][CLASS:SYNTHESIS][STATUS:RECOMMENDED][SRC:C030,C034,C038,C049]

## Nonce strategies

### Preferred: crash-safe deterministic reservation

For each `(key_id, rank_id, epoch_id)` maintain a 64-bit invocation counter and a 32-bit rank/allocator instance field, producing 96 bits. Reserve counter ranges transactionally before use; never return a range to the pool. Persist the high-water mark outside rollback-prone cache data, or bind it to the epoch key record in the key authority. After restore, clone, counter loss, or uncertainty, create a fresh epoch key and reset only within that new key domain.

A reservation record must be durable before any nonce from the range is used. Example:

```text
nonce = uint32_be(allocator_instance) || uint64_be(invocation_counter)
```

The allocator instance must be unique for the life of the key. A rank identity alone is insufficient if the same rank can be cloned or restored concurrently.

### Alternative: bounded random nonces

Generate the entire 96-bit nonce from an approved RNG, track per-key invocation counts, and rotate well below the applicable limit. This avoids a persistent counter but retains collision probability and still requires key-domain isolation across clones. For AES-GCM, NIST's RBG-based construction has a global `2^32` invocation ceiling for a key; the local HaloKV ceiling should be materially lower and operationally monitored.

[CLAIM:PFIR07-C032][CLASS:SOURCE][STATUS:SUPPORTED][SRC:NIST-SP800-38D §8.3]

### Prohibited

* Timestamp-only, process-ID-only, rank-only, path-hash-only, or truncated counters.
* Counter reset under the same key after reboot or restore.
* Sharing one `K_enc` across ranks without a formally reviewed disjoint nonce allocation.
* Deriving nonce from the plaintext CID alone; identical plaintext would repeat the nonce under the same key.
* Deliberate reuse because AES-GCM-SIV is available.

## Decrypt/accept order

1. Read fixed header into a bounded buffer; validate magic/version/suite and encoded lengths.
2. Resolve an exact key reference under caller authorization; do not try arbitrary keys or all tenant keys.
3. Reconstruct expected canonical AAD from trusted request/policy fields and compare it to the encoded AAD representation.
4. Enforce epoch floor, manifest generation, object-size, codec, and extension policy.
5. Invoke AEAD open once. On failure, emit internal reason and return `MISS_RECOMPUTE`.
6. Verify scoped CID/semantic digest after successful decryption if it is defined over canonical plaintext semantics.
7. Only then decompress/deserialize with independent resource limits.
8. Publish data to the consumer only after all checks pass.

[CLAIM:PFIR07-C059][CLASS:SYNTHESIS][STATUS:REQUIRED][SRC:C030,C034]

## Replay and freshness

A valid AEAD tag proves authenticity under its key and AAD, not recency. Include epoch and manifest generation in AAD, but compare them against trusted current state. A restored old manifest with a valid tag is rejected if its generation is below the trusted floor or a deletion tombstone supersedes it.

[CLAIM:PFIR07-C060][CLASS:SYNTHESIS][STATUS:RECOMMENDED][SRC:C030-C036]

## Failure normalization

Internal reason codes are not part of the external API. The following all become one result:

```text
MISS_RECOMPUTE
```

Unknown suite, missing key, revoked key, malformed header, oversized field, nonce-policy violation, AAD mismatch, tag failure, CID mismatch, wrong chunk, missing manifest, stale generation, and unsupported extension are logged only through a protected diagnostic channel. Avoid error-dependent network payload sizes, status codes, or retry hints that expose which key or object exists.
