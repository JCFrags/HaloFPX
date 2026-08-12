# Comparative primary-source designs

## Matrix

| Property | CachyLLama | llama.cpp state/slots | LMCache local disk | SGLang HiCacheFile | vLLM APC | HaloFPX target |
|---|---|---|---|---|---|---|
| Automatic prefix persistence | Yes, full checkpoints | No; manual/single-file mechanisms | Chunk disk tier | Yes, page L3 file backend | No, in-memory | Yes |
| Restart discovery | Header scan + index | Caller/manual | No local startup rebuild observed | Scans matching `.bin` pages | N/A | Manifest scan + object verification/rebuild |
| Prompt key | FNV + saved prefix/LCP | Saved tokens or filename | Parent-chained chunk hash | Parent-chained SHA-256 page hash | Parent hash + exact tokens + extras | Canonical prompt Merkle chain + full compatibility domain |
| Tenant isolation | `u/FNV(user_id)` | Directory/filename policy | Optional tags, no default tenant namespace | Model/rank suffix, no tenant namespace | Optional first-block cache salt | HMAC tenant/trust namespace before lookup |
| Object format | Native header + opaque state | GGSN/GGSQ + opaque state | Raw `.pt` bytes | Raw tensor `.bin` | None | Canonical fixed envelope + canonical metadata + segments |
| Atomic visibility | No | No | No | Temp + `os.replace` | N/A | Temp + verify + immutable publish + manifest CAS |
| Durable commit | Optional file fsync, unchecked; no dir sync | No explicit sync | No | No file/dir sync | N/A | Required fdatasync/fsync and directory sync |
| Checksum/authentication | None | None | None | None | Key hash only, no persisted object | Object/segment digest + keyed manifest HMAC; optional/required AEAD by policy |
| Compatibility fingerprint | FNV(model description, K/V types) | Architecture string/state shape | Model name/world/rank/dtype/tags | Model name and parallel-layout suffix | Assumes one compatible engine instance | SHA-256 of canonical complete execution manifest |
| Eviction | RAM bytes + cold count + directory count | None for files | Byte cap + policy/pinning | Byte cap, free-space watermark, LRU | Block LRU | Tenant/global bytes, cost/reuse policy, object GC |
| Multi-process safety | No | Caller responsibility | Process-local locks | Process-local lock/rank ownership | Scheduler process | Immutable objects, leases, CAS generations |
| Corruption => clean miss | Not guaranteed | Mostly structural reject; no payload digest | Short reads not rejected by reader | Short read raises; no same-length detection | N/A | Required for every failure class |

CSV version: [`tables/comparison-matrix.csv`](../tables/comparison-matrix.csv).

## LMCache local disk

<span class="badge observed">OBSERVED</span> Commit `c9439c6535503c9e17fe236da9bc88807b58c2bc`

### Key and metadata

`CacheEngineKey` serializes approximately as:

```text
model_name@world_size@worker_id@chunk_hash_hex@dtype[@tag%value...]
```

A layer key appends `@layer_id`. Chunk hashing is prefix-chained over the prior hash, current token tuple, and an extra-key tuple. The audited token database can use vLLM hash functions or Python's built-in hash; it warns that deterministic cross-process behavior requires a fixed `PYTHONHASHSEED` when the built-in path is used. Byte digests are folded to the first eight bytes for the internal integer representation. [Sources M-UTIL, M-TOK](source-matrix.md)

The key is materially better than a conversation-only hash because each chunk commits to its prefix. It still uses model name rather than an exact artifact digest, and the audited `_hash_tokens` comments state that intended extra keys for multimodal/LoRA metadata are currently ignored in that path.

### Local file behavior

- The file name is derived from the key string with `/` replaced by `-`, then `.pt`.
- `DiskCacheMetadata` tracks path, size, shape, dtype, cached positions, memory format, and pin count **in RAM**.
- The backend initializes its mutable mapping empty and does not scan `.pt` files at construction in the audited implementation. Existing bytes therefore are not independently discoverable after process restart without another catalog/control-plane mechanism.
- Buffered writes use `open(path, "wb")`; the O_DIRECT branch uses create/write flags without an observed temporary publication or `O_TRUNC`.
- No file header, checksum, version, `fsync`, directory sync, or AEAD is written.
- Reads fill a preallocated buffer with `readinto` but do not compare the returned byte count to the expected size. A short file can leave part of the destination staging buffer unchanged.
- A byte cap and cache-policy eviction are implemented, with pinning and asynchronous work queues. Locks protect in-process metadata, not a shared multi-process catalog. [Sources M-DISK, M-UTIL](source-matrix.md)

**Classification:** useful disk-tier substrate, but the audited local backend alone is not a restart-safe persistent cache.

## SGLang HiCacheFile

<span class="badge observed">OBSERVED</span> Commit `fec613184480bd6fc5bfc9967bfb24a6125f684c`

### Keying and namespace

HiRadix computes SHA-256-based page identifiers chained from the parent digest. The native extension hashes prior digest bytes plus the current page's normalized 32-bit token bytes. Only complete pages are normally materialized as L3 identities. [Sources S-UTIL, S-HASH](source-matrix.md)

The file backend appends a configuration suffix containing a sanitized model name and relevant tensor/pipeline/context-parallel rank information. Example shape:

```text
{storage_dir}/{page_hash}[.{component}]_{model}_{tp_rank}_{tp_size}_{pp...}_{cp...}.bin
```

This prevents several rank-layout collisions but does not bind exact model files, tokenizer/template, adapter digest, K/V dtype/layout manifest, or tenant identity.

### Object and recovery behavior

- Page files contain only raw tensor bytes; shape and dtype come from the currently registered host pool.
- A write reserves bytes in the eviction manager, writes a unique temporary file, then calls `os.replace(temp, final)`. On ordinary same-filesystem POSIX filesystems this gives atomic name replacement.
- No `fsync`/`fdatasync` of the temporary file or `fsync` of the directory is observed, so power-loss durability is not established.
- A read computes the expected byte size and rejects a short `readinto` result by raising `IOError`. The shown handler catches `FileNotFoundError`, not the short-read exception; higher layers must normalize this to a miss.
- Same-length bit corruption is undetectable because no checksum/header accompanies the bytes.
- Optional positive metadata caching is seeded by scanning matching files. The eviction manager separately scans existing `.bin` pages, orders initial LRU by modification time, tracks bytes, reserves in-flight writes, enforces a maximum and free-space watermark, and unlinks LRU victims.
- Locks are process-local. Immutable content naming reduces overwrite frequency, but there is no shared transaction/refcount protocol for multiple writers and evictors. [Sources S-STOR, S-LRU, S-DESIGN](source-matrix.md)

**Classification:** the strongest audited local file visibility and restart scan, but still not a self-authenticating durable object store.

## vLLM automatic prefix caching as a key baseline

<span class="badge observed">OBSERVED</span> Commit `bf578e1abdffc2d25232783ff59a3132279e6bdd`

vLLM's block hash commits to:

```text
hash(parent_block_hash, tuple(exact_block_tokens), optional_extra_keys)
```

Extra keys include multimodal identifiers/positions, LoRA name, prompt-embedding SHA-256, and an optional cache salt injected into the first block. The design caches full blocks and documents SHA-256 as the default; canonical CBOR variants are available for reproducibility. Current code initializes the root/NONE hash randomly unless a stable `PYTHONHASHSEED` is supplied, so a persistent cross-process deployment must fix that domain seed explicitly. [Sources V-DESIGN, V-KV](source-matrix.md)

Strengths to adopt:

- exact current block tokens are part of the hash input;
- parent chaining gives unambiguous prefix position;
- multimodal and adapter-related identity can enter the key;
- cache salt creates a trust-sharing boundary;
- only complete reusable units are admitted.

Limitations for this report:

- the audited APC is an in-memory block pool with LRU/refcounts, not a disk format;
- LoRA is keyed by name, not necessarily artifact digest;
- the key does not replace a full engine/model execution fingerprint;
- it defines identity, not atomic persistence, checksums, crash recovery, or encryption.

## Comparative lesson

Persistent-prefix correctness has two independent halves:

1. **Semantic address:** the key must prove that the prefix and all execution inputs produce the same KV state.
2. **Storage integrity and writer authenticity:** the bytes must be complete, the catalog pointer must be keyed-authenticated, access must be authorized, publication must be durable, and decoding must be safe.

vLLM/SGLang demonstrate stronger semantic chaining; SGLang demonstrates stronger name-level publication; llama.cpp demonstrates useful strict version/length rejection; CachyLLama demonstrates automatic tiering and workflow integration. HaloFPX combines these strengths while replacing weak persistence boundaries.
