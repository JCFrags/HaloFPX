# HaloFPX key derivation v1

All strings are UTF-8. All integers are unsigned little-endian fixed width where specified. Every digest input is domain-separated to prevent cross-protocol reuse.

## Namespace

```text
namespace_id = HMAC-SHA-256(namespace_secret,
  "halofpx.namespace/v1\0" ||
  canonical_principal_id || "\0" ||
  sharing_policy_id)
```

The namespace ID is authorization output, not user input.

## Compatibility

```text
compatibility_fp = SHA-256(
  "halofpx.compatibility/v1\0" || canonical_execution_manifest)
```

`canonical_execution_manifest` is deterministic JSON/CBOR containing all fields listed in `tables/compatibility-fingerprint.csv`, with artifact digests rather than names alone.

## Prompt chain

```text
root_0 = SHA-256(
  "halofpx.prompt-root/v1\0" || compatibility_fp || namespace_policy_digest)

root_i = SHA-256(
  "halofpx.prompt-chunk/v1\0" ||
  root_{i-1} ||
  u64le(chunk_index) ||
  u64le(start_token) ||
  u64le(end_token) ||
  canonical_u32le_token_bytes ||
  canonical_semantic_extras)
```

Semantic extras include exact adapter/media/prompt-embedding digests and positions affecting the chunk. Empty extras are encoded canonically, not omitted ambiguously.

## Cache key

```text
cache_key = SHA-256(
  "halofpx.cache-key/v1\0" ||
  namespace_policy_digest ||
  compatibility_fp ||
  root_i ||
  canonical_boundary_descriptor ||
  required_segment_policy_digest)
```

The manifest path may use the full 64-hex key or a two-level prefix. Never truncate the key for identity. Truncation is acceptable only in logs.

## Object name

```text
object_sha256 = SHA-256(all object envelope bytes)
path = objects/sha256/object_sha256[0:2]/object_sha256 + ".hkv"
```

## Canonicalization tests

Cross-language implementations must publish test vectors proving identical digests for:

- empty and non-empty token chunks;
- Unicode identifiers after the specified normalization policy;
- every integer boundary;
- multimodal extras in different order (order must be normative);
- adapter composition/order and floating scale representation;
- optional field absence versus explicit null/default;
- large model manifests with sorted artifact records.

## Manifest catalog authentication

The v1 reference catalog authenticates the canonical manifest with a namespace/tenant catalog key:

```text
manifest_auth_input =
  "halofpx.manifest-auth/v1\0" ||
  canonical_json(manifest with catalog_auth.tag_hex omitted)

catalog_auth.tag_hex = hex(HMAC-SHA-256(catalog_key, manifest_auth_input))
```

`catalog_auth.mode` and `catalog_auth.key_id` remain in the signed input. The catalog key is resolved only after principal authorization and is not stored with cache objects. Because the signed manifest commits to `object_sha256`, the HMAC transitively authenticates every byte of the content-addressed object after the whole-object digest is recomputed. Encryption remains a separate confidentiality policy; encrypted segments must use AEAD and bind the semantic metadata as additional authenticated data.
