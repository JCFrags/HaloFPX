---
title: "Rank-local cache keys"
tags: ["cache-key", "content-addressing", "privacy"]
created: 2026-07-17
updated: 2026-07-17
status: design-proposal
sources: ["KV-04", "KV-07", "SEC-01"]
related: ["Topology-Fingerprints", "Integrity-and-Corruption", "Reconnect-and-Recovery"]
---

# Rank-local cache keys

## Two-level identity

Do not use one overloaded “cache key.” HaloKV separates a semantic lookup key from an immutable object ID.

```text
logical_lookup_key = HMAC-SHA256(
  tenant_cache_key,
  CanonicalPrefixIdentity
)

page_id = SHA-256(
  "halokv-page-v1" || CanonicalPageHeader || page_bytes
)
```

The HMAC prevents a tenant from probing another tenant’s raw token/prompt hashes. Cross-tenant deduplication should be disabled by default because object-existence and timing can leak information. The page digest provides end-to-end identity and corruption detection; encryption scope can still keep identical plaintext pages from sharing ciphertext across tenants.

## Canonical prefix identity

The identity must cover every input that can alter KV values, not only token IDs:

- token IDs and canonical position/mask data;
- digest of prompt embeddings or soft prompts when supplied directly;
- multimodal input digests and preprocessing revisions;
- model weights revision and relevant configuration;
- adapter/LoRA identifiers and weight digests;
- tokenizer/input-contract revision where token interpretation can differ;
- RoPE/scaling, sliding-window, attention-mask, and position policy;
- quantization/calibration and KV dtype;
- cache salt and tenant namespace;
- deterministic feature flags that alter the forward pass.

Do not include request IDs, physical host IDs, wall-clock timestamps, or transport addresses in the semantic identity.

## Canonical page header

```json
{
  "format": "halokv-page-v1",
  "tenant_scope_id": "opaque-16-byte-id",
  "model_fingerprint": "sha256",
  "topology_fingerprint": "sha256",
  "session_generation": 7,
  "logical_rank": 1,
  "shard_descriptor": "layers=24..47;heads=16..31",
  "sequence_id": 0,
  "position_start": 4096,
  "position_count": 256,
  "layer_start": 24,
  "layer_count": 1,
  "kv_kind": "K_AND_V",
  "dtype": "fp8_e4m3fn",
  "layout": "engine-abi-17",
  "shape": [2, 1, 16, 256, 128],
  "uncompressed_length": 2097152
}
```

Canonical serialization must be specified byte-for-byte. Recommended choices are deterministic protobuf with a separately versioned canonicalization contract or canonical CBOR/JSON. Unknown fields may be retained for transport but cannot silently enter a digest unless the digest version defines them.

## Rank-local namespace

A convenient logical path for indexes is:

```text
/tenant/{tenant_scope}/model/{model_fp}/topology/{topology_fp}/
/session/{session_id}/generation/{g}/epoch/{e}/rank/{r}/segment/{segment}
```

This path is an index, not the page’s storage identity. The immutable page object is addressed by digest and can be referenced from multiple retained checkpoints within the same authorized deduplication scope.

## Relocation and re-sharding

A logical rank can move to another physical node without changing page IDs when the exact-reuse fingerprint and shard descriptor remain identical. Re-sharding from two ranks to one changes topology and normally invalidates exact page reuse. A conversion tool may be implemented, but it must be explicit, versioned, all-or-nothing, and verified against recomputation; otherwise rebuild from tokens.

## Collision and ambiguity policy

- The digest algorithm and digest length are part of the object format version.
- Object creation is `put-if-absent`; an existing digest with different length or bytes is a critical integrity incident.
- Index keys include generation and topology, so an old checkpoint cannot shadow a new one.
- Page coordinates and rank are inside the digest header, preventing a valid byte string from being reinterpreted at another tensor coordinate.
