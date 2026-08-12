# Crash recovery, immutable writes, and manifests

## Object write protocol

HaloKV objects are immutable after publication. A safe local write sequence is:

1. Authorize principal and scope; obtain exact rank-epoch keys.
2. Canonicalize semantics and compute scope-specific CID.
3. Reserve a nonce durably before first use.
4. Construct final AAD, including object size/chunk/manifest fields.
5. AEAD-seal to a newly created temporary file in the target filesystem.
6. Write complete header, AAD, ciphertext, and tag; reject short writes.
7. `fsync`/equivalent the file according to the filesystem durability contract.
8. Atomically rename into a non-overwriting content-object path.
9. `fsync`/equivalent the parent directory when required by the platform.
10. Publish the authenticated manifest/index update last using the same immutable/atomic pattern.

A temporary orphan, missing object, truncated object, duplicate object, or manifest referencing an absent object is a miss/recompute condition. Recovery may garbage-collect unreferenced complete objects after an age threshold.

## Crash matrix

| Crash point | Expected state | Recovery action |
|---|---|---|
| Before nonce reservation commits | No nonce may have been used | Retry reservation or start new epoch if uncertain |
| After reservation, before encryption | Reserved nonce/range is burned | Skip it; never return to pool |
| During object write | Temp/truncated file | Never index; delete/quarantine after bounded inspection |
| After object fsync, before rename | Complete temp file | Validate and rename only through recovery policy, otherwise delete |
| After rename, before directory durability | Presence may be filesystem-dependent | Startup scan treats absent/unknown as miss; do not infer nonce reuse |
| After object publish, before manifest publish | Orphan complete object | Safe but unreachable; garbage-collect later |
| During manifest write | Old manifest remains authoritative | Reject temp/invalid generation; recompute missing entries |
| After new manifest publish, before old cleanup | Both generations may exist | Trusted generation pointer/floor chooses current; old generation is non-serving |

## Nonce state recovery

Nonce state is more sensitive than cache availability. If the system cannot prove which nonces were consumed under a key, it creates a new epoch key. It does not scan object files and infer a safe next counter, because lost/unpublished ciphertext, backup copies, concurrent clones, or partially durable writes may not be visible.

[CLAIM:PFIR07-C033][CLASS:SOURCE][STATUS:SUPPORTED][SRC:NIST-SP800-38D §8.2.1]

## Manifest model

A manifest is immutable and generation-addressed. A small trusted pointer or authority record names the current accepted generation and minimum epoch. The manifest contains:

* authority/tenant/project/prefix/namespace/sharing identifiers;
* rank and epoch policy;
* generation, predecessor, creation time, optional expiration;
* source revision or model/build identity;
* ordered object/chunk entries with scoped CID, length, key reference, and object type;
* tombstone or supersession information where applicable;
* critical format/policy extensions;
* MAC/AEAD tag or publisher signature.

The full design schema is [`schemas/manifest.cddl`](../schemas/manifest.cddl).

## Authentication order on startup

1. Load trusted policy/epoch floor from the authority, not the cache directory alone.
2. Locate candidate current manifest without trusting path metadata.
3. Bound and parse only the fixed outer framing.
4. Resolve exact authorized manifest key/signing identity.
5. Authenticate the manifest; reject unknown critical extensions.
6. Enforce generation/epoch/source policy and deletion tombstones.
7. Build a serving index only from authenticated entries.
8. Validate each object lazily on read or eagerly according to declared restore/startup policy.
9. Any invalid entry is removed from the serving index and becomes miss/recompute.

## Filesystem and block-layer recovery

Object-level atomicity assumes documented filesystem semantics. Test the exact filesystem, mount options, storage hardware, virtualization layer, and power-failure model. Lower layers affect what failures appear:

* ordinary dm-crypt does not authenticate corruption;
* dm-integrity journal mode can maintain data/tag atomicity, while direct mode can expose mismatch after a crash;
* an unencrypted integrity journal may leak recent write behavior;
* LUKS2 reencryption resilience mode determines recovery after interrupted volume transformation;
* header backups are sensitive and must match the intended recovery point.

[CLAIM:PFIR07-C017][CLASS:SOURCE][STATUS:SUPPORTED][SRC:LINUX-DMINTEGRITY-7.2RC3 §Modes]

[CLAIM:PFIR07-C018][CLASS:SOURCE][STATUS:SUPPORTED][SRC:LINUX-DMINTEGRITY-7.2RC3 §Security considerations]

[CLAIM:PFIR07-C013][CLASS:SOURCE][STATUS:SUPPORTED][SRC:LUKS2-SPEC-1.1.4 §7; CRYPTSETUP-2.8.6 cryptsetup-reencrypt]

## Rebuild policy

Because the cache is non-authoritative, ambiguous recovery favors discard/recompute. Do not repair tags, rewrite AAD, guess a key, patch a CID, or “salvage” partially authenticated plaintext in place. A recovery tool may collect ciphertext and metadata into quarantine for analysis, then remove the serving reference.
