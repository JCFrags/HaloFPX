# Observed CachyLLama implementation

<span class="badge observed">OBSERVED</span> Source commit `6be745998f568e379ea197fcf827baec73ff9940`  
<span class="badge inference">INFERENCE</span> Filesystem and ABI consequences are marked explicitly  

## Architecture

CachyLLama adds a server-level page manager around a low-level SSD cache. The principal path is:

```text
request tokens
  -> conversation or user routing
  -> per-scope kv_ssd_cache
  -> prefix/slot checkpoint lookup
  -> cold file read or warm/hot RAM copy
  -> llama_state_seq_set_data_ext()
  -> reuse on success; normal prefill path on miss/failure
```

Each checkpoint contains a full serialized target sequence state and may append draft/MTP and speculative-implementation blobs. Hot and warm tiers hold the same concatenated bytes in RAM; cold means the checkpoint remains only on SSD. The system-prompt cache is a separate global pool with a different magic and retention policy. [Sources C-KV-H, C-KV-CPP, C-SRV, C-SYS-H, C-SYS-CPP](source-matrix.md)

![Observed CachyLLama lifecycle](../diagrams/observed-cachyllama-lifecycle.svg){.diagram}

## Keys and namespaces

### Anonymous conversations

The design document derives a conversation identity from an FNV-1a hash of up to the first 1,024 task tokens. The cache path is:

```text
{base_path}/{conv_hash_16hex}/
    index.bin
    ckpt-{sequential_id}.bin
```

Within a conversation directory, lookup compares the incoming token sequence with each stored `token_prefix` and chooses a best candidate by longest common prefix and recency/turn criteria. At most 4,096 prefix tokens are persisted for comparison. A global anonymous continuation scan can inspect top-level hexadecimal conversation directories and accept a fuzzy overlap threshold. [Sources C-USER, C-KV-CPP, C-PM](source-matrix.md)

### Explicit users

When a request carries a user ID, the page manager hashes its bytes with unkeyed FNV-1a and routes to:

```text
{base_path}/u/{fnv1a64(user_id)_16hex}/
```

The global anonymous scanner skips the `u/` subtree, and user requests do not fall back across users. This is useful logical isolation, but it is not a cryptographic namespace: low-entropy user identifiers can be guessed offline, and the source logs raw user IDs in some paths. HaloFPX should use an HMAC-derived tenant identifier and avoid raw identifiers in logs. [Sources C-USER, C-PM](source-matrix.md)

### System prompts

The separate system cache uses:

```text
{model_dir}/sys-{fnv1a64(system_tokens)_16hex}.bin
```

It defaults to eight entries and thirty days of unused-age retention. It verifies exact saved tokens only for the stored prefix, capped at 4,096; a longer prompt relies on the 64-bit FNV hash for the remainder. [Sources C-SYS-H, C-SYS-CPP](source-matrix.md)

## Compatibility fingerprint

The page manager computes a 64-bit FNV-1a value over:

1. the bytes returned by `llama_model_desc(model)`; and
2. the target context's K and V cache type values.

The code comment mentions build identity, but the observed calculation does not append a build commit. It also does not explicitly commit to model-file/shard digests, tokenizer, vocabulary normalization, chat template, GGUF overrides, adapters, RoPE/context parameters, draft model, speculative algorithm, or serialized-state ABI. A mismatch is correctly treated as a miss, but a match is not a complete proof of compatibility. [Source C-PM](source-matrix.md)

## Checkpoint v3 on-disk layout

The implementation writes the native `kv_ssd_record` object directly, followed by target, draft, and speculative bytes. The table below describes the common LP64 little-endian layout used by the included validator. It is **not** a portable upstream wire-format guarantee.

| Offset | Size | Field | Validation observed in normal recovery/load |
|---:|---:|---|---|
| 0 | 4 | magic `0x4b565243` (`KVRC`) | Header scan checks magic |
| 4 | 4 | version, written as `3` | Record scan/load does not consistently reject a different record version |
| 8 | 8 | checkpoint `id` | Compared to requested ID during cold promotion |
| 16 | 4 | `slot_id` | Trusted as metadata |
| 20 | 4 | `pos_min` | Trusted as metadata |
| 24 | 4 | `pos_max` | Trusted as metadata |
| 28 | 4 | implicit ABI padding | Persisted unintentionally |
| 32 | 8 | `n_tokens` | Used for matching/restore metadata |
| 40 | 4 | `turn_created` | Used by cold eviction ordering |
| 44 | 4 | implicit ABI padding | Persisted unintentionally |
| 48 | 8 | target `data_size` | Used for allocation and reads; no cryptographic binding |
| 56 | 8 | `token_hash` | FNV-1a identity hint, not integrity |
| 64 | 4 | `token_count` | No format-level bound/checksum during scan |
| 68 | 4 | implicit ABI padding | Persisted unintentionally |
| 72 | 8 | `compat_hash` | Compared with current cache fingerprint |
| 80 | 16,384 | `token_prefix[4096]` | Used for LCP matching |
| 16,464 | 8 | `dft_data_size` | Optional appended draft bytes |
| 16,472 | 8 | `spec_data_size` | Optional appended speculative bytes |
| 16,480 | variable | target, draft, speculative payloads | Exact read loops, but no digest or AEAD |

Machine-readable schema: [`schemas/cachyllama-v3-lp64le.ksy`](../schemas/cachyllama-v3-lp64le.ksy).

## Index layout

`index.bin` is one 120-byte native `kv_ssd_index_header`:

| Offset | Size | Field |
|---:|---:|---|
| 0 | 4 | magic `0x4b564944` (`KVID`) |
| 4 | 4 | version `3` |
| 8 | 8 | `next_id` |
| 16 | 8 | `compat_hash` |
| 24 | 96 | twelve reserved `uint64_t` values |

The index does not enumerate records; startup reconstructs the map by scanning `ckpt-*.bin`. The index mainly preserves the next sequential ID and compatibility value. [Sources C-KV-H, C-KV-CPP](source-matrix.md)

## System-prompt v1 layout

The system cache writes a 16,440-byte native header followed by one state payload:

| Offset | Size | Field |
|---:|---:|---|
| 0 | 4 | magic `0x4b565359` (`KVSY`/system record constant) |
| 4 | 4 | version `1` |
| 8 | 8 | FNV token hash |
| 16 | 4 | total system `n_tokens` |
| 20 | 4 | payload `data_size` (32-bit) |
| 24 | 8 | compatibility hash |
| 32 | 8 | creation time |
| 40 | 8 | last-used time |
| 48 | 4 | access count |
| 52 | 4 | saved token count |
| 56 | 16,384 | `token_prefix[4096]` |
| 16,440 | variable | serialized state |

The 32-bit persisted size can truncate a `size_t` value above 4 GiB while the writer still emits the full buffer. No checksum or AEAD is present. [Sources C-SYS-H, C-SYS-CPP](source-matrix.md)

## Write and publication behavior

### Checkpoint

The observed sequence is:

1. allocate `id = next_id++` under the cache mutex;
2. open final `ckpt-{id}.bin` with create/write/truncate;
3. write the native header, target payload, and optional payloads with exact positional-write loops;
4. optionally call `fsync(fd)` unless `no_fsync` is configured;
5. close the file;
6. update in-memory maps and write `index.bin` directly with create/write/truncate.

The return value of `fsync` is not used to reject the store, and no parent-directory `fsync` occurs. There is no same-directory temporary file, immutable object name, `rename`, checksum, or post-write verification. [Source C-KV-CPP](source-matrix.md)

### System prompt

System entries are also opened at their final `sys-*.bin` path with truncation and written in place. Updating an existing entry mutates RAM first; if the disk rewrite fails, the method logs the problem but can retain/return the in-memory success path. Access-time/count updates made on lookup are not durably rewritten on every hit. [Source C-SYS-CPP](source-matrix.md)

### Consequence

<span class="badge inference">INFERENCE</span> `fsync` can make successfully written bytes durable, but it cannot make a final-name, truncating multi-write sequence atomic. A crash after truncation and before the last payload byte can leave a file visible with a complete header and short body. Without directory synchronization, a successful rename/create/unlink would not itself be guaranteed durable across power loss even if the file were synced.

## Startup, reads, and crash recovery

- Initialization creates directories and reads `index.bin`. An invalid/missing index clears the in-memory map and resets `next_id` to 1.
- It scans filenames matching `ckpt-*.bin`, reads one full record header, and checks the record magic before adding metadata.
- The observed scan does not first prove `header_size + target + draft + spec == file_size`, bound each length, validate all token-count invariants, or authenticate the body.
- Cold promotion reopens the file, checks magic and ID, allocates a combined vector using persisted lengths, and uses exact reads. A short body therefore fails at load time, but a corrupted large length can influence allocation first.
- A missing file removes its index entry; several other errors simply return false. No quarantine directory or durable bad-object tombstone is maintained.
- A compatibility mismatch returns false and is treated as a cache miss by the server wrapper.
- Target-state import failure returns false. Draft-state restore failure is logged and tolerated because the draft/MTP path can catch up; this is a deliberate partial policy for an optional acceleration component, not permission to partially restore the target state. [Sources C-KV-CPP, C-SRV, C-PM](source-matrix.md)

### Index-loss collision

<span class="badge inference">INFERENCE</span> The recovery scan reconstructs records but does not advance `next_id` from the maximum discovered checkpoint ID in the observed path. If `index.bin` is lost or rejected while `ckpt-1.bin` remains, a subsequent store can allocate ID 1 and truncate that existing file. The validation suite includes a static check and a documented recovery requirement; it does not run the upstream server.

## Corruption and partial-write behavior

| Condition | Observed format response | Safety assessment |
|---|---|---|
| Header shorter than 16,480 bytes | Scanner cannot read full header; entry is skipped | Clean miss likely |
| Complete header, truncated body | Entry can be indexed; exact read later fails | Delayed miss; metadata was trusted before total-length proof |
| Wrong record magic | Rejected | Clean miss |
| Wrong record version with valid magic | Not consistently rejected on record scan/load | Unsupported layout can be interpreted as v3 |
| Compatibility mismatch | Rejected | Correct miss |
| Same-length payload bit flip | No format-level detection | May reach engine importer; unacceptable for trusted reuse |
| Corrupted size field | Can trigger oversized allocation/read attempt | Must be bounded before allocation |
| Corrupted `token_count` above 4096 | Persisted array access/metadata construction is not protected by an authenticated header | Parser-hardening required |
| Truncated/corrupt `index.bin` | Index rejected; checkpoint scan attempted; `next_id` resets | Recovery possible, but ID-reuse risk |
| FNV collision | Exact saved prefix reduces risk for first 4,096 tokens; long suffix/system remainder can alias | FNV is not a compatibility or security digest |

## Eviction and retention

- Hot and warm RAM tiers have byte budgets and turn-based demotion.
- Cold file eviction sorts cold checkpoints by `turn_created` and enforces a checkpoint-count cap. It is not a disk-byte quota.
- The page manager limits anonymous and user cache objects through separate maps; the observed implementation can therefore retain up to the configured cap in each class rather than one combined global cap.
- Conversation directory eviction uses directory modification time among cache objects known to the process.
- System entries use entry-count LRU and unused-day expiration. The `max_hot_bytes` field is declared but is not the primary enforced system-cache bound in the audited path.

A count-only cold policy is poorly correlated with bytes because full context states can range from hundreds of MiB to many GiB.

## Concurrency

| Scope | Observed mechanism | Gap |
|---|---|---|
| One `kv_ssd_cache` | `std::mutex` around most map/store/load operations | No process-shared lock or transaction |
| Page manager | `shared_mutex` around cache maps and routing | Files can still be shared by unrelated processes |
| Metadata access | `kv_ssd_get_meta` returns a pointer after lock scope | Caller can observe invalidated storage after concurrent mutation |
| Slot prefetch | Iteration and helper locking are not one atomic snapshot | Mutation race window |
| System cache | Mutex-protected map, but `find()` returns an entry pointer and `load()` copies later | Pointer lifetime can race with store/eviction |
| Filesystem | Sequential final-name files and index | No lease, CAS generation, or immutable object discipline |

## What CachyLLama gets right

- It treats compatibility mismatch and target import failure as a miss rather than forcing reuse.
- Exact read/write loops handle interruptions and large transfers.
- Anonymous and explicit-user namespaces are separated in path routing.
- Optional draft/speculative data is distinct from mandatory target state.
- Kernel readahead and RAM tiers are performance-aware.
- Startup scanning can recover files that were not represented by a complete index.

These strengths should be retained behind a redesigned storage boundary rather than discarded.
