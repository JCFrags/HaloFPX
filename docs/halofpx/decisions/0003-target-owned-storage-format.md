# ADR-0003: target-owned storage format

- Status: accepted for L02
- Date: 2026-07-17
- Resolves: `OPEN-FMT-01`

## Decision

HaloFPX owns a versioned format. CachyLLama `KVRC`/`KVSM`/`KVPG`, llama-ai,
and existing RPC cache formats are not trusted server ABI and cannot produce a
hit. A future legacy tool may only inspect them offline, read-only, with strict
bounds; migration means validate into a typed intermediate form and publish a
new HaloFPX object, never relabel donor bytes.

Compatibility metadata uses deterministic CBOR under RFC 8949 and SHA-256 with
domain-separated preimages. Promotion requires two independent encoders and
checked-in golden vectors to produce identical canonical bytes and hashes.
Until then, the format is specified but no reader or writer is enabled.

An immutable object frame is:

```text
magic[8] = 48 41 4c 4f 4f 42 4a 01
domain_len:u16be | domain ASCII
type_len:u16be   | registered_type ASCII
payload_len:u64be
payload[payload_len]
```

The domain is exactly `halofpx.object.v1`. The object identifier is SHA-256 of
the entire frame. The exact closed integer-key field registry and all 16
compatibility-component preimages are in
`../contracts/context-store-v1.cddl`. Its authenticated manifest envelope
records an authentication input plus its tag. The input binds the complete
manifest body, key ID, algorithm, and key generation. The body records store/lineage identity, complete
compatibility metadata, scope/policy epoch, generation/predecessor, topology/
rank ownership, state profile, object lengths/digests/types/codecs, producer,
and durability mode. All integers use checked arithmetic before allocation.

```text
compat_root = SHA-256("halofpx.compat.v1\0" || DCBOR(compatibility-manifest))
manifest_tag = HMAC-SHA-256(K_manifest,
  "halofpx.manifest-auth.v1\0" || DCBOR(manifest-auth-input-v1))
manifest_digest = SHA-256(
  "halofpx.manifest.v1\0" || DCBOR(authenticated-manifest-v1))
K_manifest = HMAC-SHA-256(K_master,
  "halofpx.manifest-key.v1\0" || key_id_len:u16be || key_id ||
  store_uuid || namespace_id || key_generation:u64be)
```

Each compatibility field is computed as
`SHA-256("halofpx.compat-component.v1\0" || label_len:u16be || label ||
DCBOR(the corresponding closed *-input-v1 submanifest))`. Labels are exactly
the 16 JSON registry strings, and the CDDL maps each label to its input type.
Arrays are semantically ordered; unordered source maps are first sorted by the
canonical encoded key, duplicates are rejected, every registered ID must exist
in the profile's closed registry, and typed values use the CDDL tag union.
Output-affecting binary64 values are encoded as exactly eight big-endian
IEEE-754 bytes, not CBOR floats, so deterministic CBOR's shortest-float rule
cannot change their identity. NaN and infinity are rejected; signed zero and
all finite bit patterns retain exact identity.

`K_master` is protected service state and is never stored in the cache root.
The reader selects a known non-revoked key ID/generation, verifies the manifest
tag in constant time before decoding any referenced payload or mutating live
state, then verifies compatibility and every object hash. Unknown/revoked keys,
tag failure, or key-generation rollback is a miss and security event. Rotation
creates a new namespace/key generation; old material is read-disabled unless
an authenticated offline migration republishes it.

V1 parsers reject noncanonical CBOR, duplicate map keys, invalid UTF-8 or NUL,
unknown keys or critical fields, unknown object types/codecs/profiles, trailing bytes,
wrong version/domain, wrong or oversized length, excessive nesting/count, and
unexpected extra objects. Initial limits are: manifest at most 1 MiB, nesting
at most 16, 128 components, 16 KiB per text/byte-string metadata value, and no
indefinite-length items. Payload limits are deployment-configured but must be
positive, bounded by both quota and storage reserve, and checked before I/O.

Paths are derived only from validated lowercase hexadecimal identifiers under
fixed roots. Readers reject symlinks, reparse points, device files, traversal,
alternate data streams, type swaps, and objects whose opened identity changes
during validation. Indexes are rebuildable accelerators and never authority.

## Versioning

Unknown major, hash, canonicalization, codec, or required field is a miss.
There are no optional or ignorable extension fields in v1; exact CDDL map
closure is mandatory. Format migration is side-by-side
republication by a separately pinned tool; semantic compatibility changes are
recomputation, not byte migration.
