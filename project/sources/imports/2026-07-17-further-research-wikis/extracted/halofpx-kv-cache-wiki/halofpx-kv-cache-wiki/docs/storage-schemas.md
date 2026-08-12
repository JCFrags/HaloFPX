# Storage schemas

## Observed CachyLLama schemas

<span class="badge observed">OBSERVED</span> The upstream implementation writes native structs. The supplied Kaitai files are an explicit LP64/little-endian interpretation for analysis and validation, not a claim of portable upstream serialization.

| File | Header bytes | Schema | Payload |
|---|---:|---|---|
| `ckpt-{id}.bin` | 16,480 | [`cachyllama-v3-lp64le.ksy`](../schemas/cachyllama-v3-lp64le.ksy) | target state, optional draft state, optional speculative state |
| `index.bin` | 120 | [`cachyllama-index-v3-lp64le.ksy`](../schemas/cachyllama-index-v3-lp64le.ksy) | none |
| `sys-{hash}.bin` | 16,440 | [`cachyllama-system-v1-lp64le.ksy`](../schemas/cachyllama-system-v1-lp64le.ksy) | one system-prompt state blob |

The observed formats lack:

- a header length field;
- canonical endianness/packing declaration;
- exact total object length;
- header checksum;
- payload or segment digest;
- compression/encryption identifiers;
- nonce/key ID/authentication tag;
- canonical compatibility manifest;
- migration metadata.

## HaloFPX object v1

<span class="badge recommended">HALOFPX RECOMMENDATION</span>

### Fixed header

All integer fields are unsigned little-endian. The fixed header is exactly 104 bytes.

| Offset | Type | Field | Rule |
|---:|---|---|---|
| 0 | `char[8]` | magic | exactly `HFPXKVC1` |
| 8 | `u16` | major | exactly `1` for this reader |
| 10 | `u16` | minor | supported compatible minor |
| 12 | `u32` | header length | exactly `104` in v1 |
| 16 | `u64` | metadata length | bounded before read/allocation |
| 24 | `u64` | payload length | bounded before read/allocation |
| 32 | `u32` | segment count | bounded and equals metadata table count |
| 36 | `u32` | flags | only known flag bits accepted |
| 40 | `byte[32]` | metadata SHA-256 | digest of exact canonical metadata bytes |
| 72 | `byte[32]` | payload SHA-256 | digest of exact stored payload bytes |

Exact file size must equal `104 + metadata_len + payload_len`; checked addition must not overflow the implementation's size type.

Schema: [`halofpx-object-v1.ksy`](../schemas/halofpx-object-v1.ksy).

### Canonical metadata

The v1 validator uses canonical UTF-8 JSON produced with sorted keys, no insignificant whitespace, and UTF-8 characters preserved. Production may use canonical CBOR if every implementation shares one normative profile; the wire format must identify which one.

Required metadata shape:

```json
{
  "schema": "halofpx.kv.object/v1",
  "cache_key_sha256": "64 hex",
  "compatibility_fingerprint_sha256": "64 hex",
  "prompt_root_sha256": "64 hex",
  "boundary": {"kind": "token_page", "start_token": 0, "end_token": 4096},
  "engine": {"family": "llama.cpp", "state_abi": "..."},
  "segments": [
    {
      "name": "target",
      "required": true,
      "offset": 0,
      "stored_length": 123,
      "logical_length": 123,
      "stored_sha256": "64 hex",
      "codec": "none",
      "encryption": "none",
      "role": "target_sequence_state"
    }
  ],
  "created_unix_ns": 0,
  "writer_id": "uuid",
  "policy": {"partial_reuse": "complete_prefix_only"}
}
```

Segment offsets are relative to the start of the payload. In v1 they must be sorted, non-overlapping and cover the stored payload exactly; this avoids unauthenticated gaps or ambiguous bytes. Required segments must be present exactly once.

CDDL: [`halofpx-cache-object-v1.cddl`](../schemas/halofpx-cache-object-v1.cddl).

### Encrypted segments

Recommended segment metadata when application-level encryption is enabled:

```json
{
  "encryption": "aes-256-gcm",
  "key_id": "tenant-key/2026-07",
  "nonce_b64": "unique 96-bit nonce",
  "tag_length": 16,
  "aad_profile": "halofpx.segment-aad/v1"
}
```

The AEAD additional authenticated data must commit to the immutable object's semantic metadata: schema, cache key, compatibility fingerprint, prompt root, boundary, segment name/role, stored/logical lengths, codec, key ID and generation-independent policy. The stored segment digest is over ciphertext for early corruption detection; AEAD verification is still mandatory before use.

## HaloFPX manifest v1

A manifest is the only reachability pointer from a semantic cache key to an object. It is small, canonical, generationed, keyed-authenticated and atomically replaced. The reference v1 profile uses HMAC-SHA-256 over every canonical manifest field except the tag itself.

Required fields:

```json
{
  "schema": "halofpx.kv.manifest/v1",
  "generation": 1,
  "namespace_id": "64 hex HMAC-derived ID",
  "engine_family": "llama.cpp",
  "cache_key_sha256": "64 hex",
  "compatibility_fingerprint_sha256": "64 hex",
  "prompt_root_sha256": "64 hex",
  "object_sha256": "64 hex",
  "object_size": 1234,
  "created_unix_ns": 0,
  "last_access_unix_ns": 0,
  "verified_prefix_tokens": 4096,
  "required_segments": ["target"],
  "encryption_policy": "none",
  "state": "committed",
  "catalog_auth": {
    "mode": "hmac-sha256",
    "key_id": "tenant-catalog/2026-07",
    "tag_hex": "64 hex"
  }
}
```

JSON Schema: [`halofpx-manifest-v1.schema.json`](../schemas/halofpx-manifest-v1.schema.json).  
Example: [`sample-manifest.json`](../schemas/sample-manifest.json).

The authentication input is:

```text
HMAC-SHA-256(catalog_key,
  "halofpx.manifest-auth/v1\0" ||
  canonical_json(manifest with catalog_auth.tag_hex omitted))
```

The HMAC transitively authenticates the referenced whole-object digest. Plain SHA-256 in the object envelope detects accidental corruption but is not keyed authenticity by itself. Authorization remains a separate pre-lookup decision. Production keys must come from a protected key manager and must not be stored under the cache root; the included fixture key is public test material.

### Manifest/object binding

A read must prove all of the following:

```text
manifest.catalog_auth verifies under the authorized catalog key
manifest.namespace_id == authorized.namespace_id
manifest.cache_key == requested.cache_key
manifest.compatibility_fp == current.compatibility_fp
manifest.prompt_root == requested.prompt_root
manifest.object_size == stat(object).size
SHA256(object_bytes) == manifest.object_sha256 == object_filename_digest
object.metadata.cache_key == manifest.cache_key
object.metadata.compatibility_fp == manifest.compatibility_fp
object.metadata.prompt_root == manifest.prompt_root
required segment names are present and verified
transactional engine import succeeds before HIT_VERIFIED
```

Duplication is intentional defense in depth: a single corrupted or aliased pointer does not silently redirect state. The offline reference validator returns `CATALOG_ENTRY_VALID` when the authenticated catalog entry is internally consistent but current-request bindings were not supplied; it returns `IMPORT_CANDIDATE_VALID` only after all such bindings match. Only the integrated engine adapter may return `HIT_VERIFIED`.

## Key derivation and path safety

Detailed canonical inputs are in [`halofpx-key-derivation.md`](../schemas/halofpx-key-derivation.md).

Filesystem rules:

- create root and tenant directories mode `0700`, files `0600` unless a stricter service account policy applies;
- use directory file descriptors plus `openat2`/equivalent resolution controls where available;
- reject symlinks, `..`, NULs and non-canonical names;
- object names are lowercase fixed-length hex generated internally, never direct user input;
- manifests are placed only after namespace authorization;
- temporary files remain in the same filesystem as the target object/manifest;
- encryption keys never appear in paths or object metadata beyond non-secret key IDs.

## Schema evolution

Every envelope and manifest has an explicit schema/major version. Evolution rules:

- adding optional metadata requires a new minor profile and canonical default semantics;
- changing field meaning, digest domain, encryption AAD, segment layout or compatibility definition requires a new major version;
- readers reject unknown critical fields/flags;
- old-to-new conversion creates a new object and manifest generation;
- validators retain hard caps independently of schema claims.
