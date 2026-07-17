# Observed current llama.cpp functionality

<span class="badge observed">OBSERVED</span> Source commit `86d86ed4396b4130922f7b9af26e3d9fc11a591b`

## Mechanisms that are often conflated

Current llama.cpp exposes several different cache/state features. They are not one persistent-prefix-cache subsystem.

| Mechanism | Persistence | Addressing | Intended use |
|---|---|---|---|
| `llama_state_save_file` / `llama_state_load_file` | One full state file | Caller filename; saved token vector | Resume a context/session |
| `llama_state_seq_save_file` / `llama_state_seq_load_file` | One sequence state file | Caller filename plus sequence ID at save/load | Move or restore a sequence state |
| Server slot save/restore endpoint | Manual files below `--slot-save-path` | Validated user-supplied filename | Operator/client-triggered slot snapshot |
| CLI prompt-cache file | One configured file, optionally read-only/all prompts | Configured filename | Faster startup/reuse for a known prompt workflow |
| Server prompt cache / `--cache-prompt` | RAM only | Prompt tokens and slot state | Reuse while the server process is alive |
| `--cache-reuse N` | RAM KV shifting/reuse | Similar chunks in active slots | In-process optimization |

None of these is an automatic, multi-entry, tenant-aware, restart-scanned, content-addressed SSD prefix cache with quotas and integrity metadata. [Sources L-H, L-CTX, L-MMAP, L-SRV](source-matrix.md)

## File formats and compatibility

### Full session state

The public header identifies full session files with magic `GGSN` and session version **9**. The implementation writes:

1. magic and version;
2. a 32-bit token count;
3. the saved token IDs;
4. serialized context state.

The reader rejects a wrong magic/version, refuses a token count larger than caller capacity, reads exact token bytes, and requires the state decoder to consume the remaining file. These checks are materially stronger than a magic-only header scan. [Sources L-H, L-CTX](source-matrix.md)

### Sequence state

Per-sequence state files use magic `GGSQ` and sequence-state version **2**. They carry a sequence snapshot through the state serializer rather than a general persistent cache object with its own catalog, tenant namespace, or retention policy.

### Compatibility proof

The state stream explicitly writes and checks a model architecture string. The source contains a TODO to add more model-specific compatibility information. Internal state modules also impose shape/size expectations while decoding, but the file does not cryptographically commit to exact model files, tokenizer/template, adapters, runtime parameters, K/V serialization ABI, or build identity.

Consequently, a successful parse is not equivalent to a HaloFPX full compatibility fingerprint.

## Write semantics

The state save helpers create `llama_file` with mode `"wb"`, which opens/truncates the destination and writes through `fwrite`; destruction closes it with `fclose`. The audited path does not publish through a temporary file and rename, call `fsync`/`fdatasync`, synchronize the directory, append a checksum, or authenticate metadata. [Sources L-CTX, L-MMAP](source-matrix.md)

<span class="badge inference">INFERENCE</span> A process crash or power loss can leave a visible partial state file. Strict length decoding will usually reject it on restore, which is safer than accepting it, but the write is still neither atomic nor durably committed.

## Server slot files

When a slot save path is configured, the server validates the requested filename, constructs a path below that directory, and calls sequence-state save/load. On restore, a zero-byte/read failure result is reported as an invalid slot file or insufficient capacity; the server does not install the restored prompt state. Success replaces the slot's logical prompt tokens with those read from the file. [Source L-SRV](source-matrix.md)

Important boundaries:

- saving is explicit through the slot endpoint, not automatic on every useful prefix;
- filenames are names, not prompt-content keys;
- there is no built-in manifest/catalog scan, LRU disk eviction, per-tenant namespace, checksum, quarantine, or migration engine;
- the current server path saves the target sequence state, not a CachyLLama-style target+draft+spec object bundle;
- a failed manual restore returns an error. A later ordinary inference request may recompute, but the restore endpoint itself is not an automatic recomputation transaction.

## Invalid-state behavior

| Condition | Observed behavior | HaloFPX interpretation |
|---|---|---|
| Wrong magic or version | Read fails | `MISS_RECOMPUTE` |
| Saved token count exceeds provided capacity | Read fails | `MISS_RECOMPUTE`; caller may retry with a larger trusted buffer, not trust the file blindly |
| Truncated session body | State read does not consume expected remainder; failure | `MISS_RECOMPUTE` |
| Same-length payload mutation | No file-level checksum; may be caught by state parser or may not | Must be authenticated before import |
| Architecture string mismatch | State decode rejects | `MISS_RECOMPUTE` |
| Newer/older version | Exact version rejection | Safe miss, but no migration path |
| Manual slot restore error | Error response; logical restore not installed | Correct no-hit behavior |

## Comparison with CachyLLama

llama.cpp's state readers provide stricter file magic/version and total-consumption checks. CachyLLama adds the missing automatic indexing, namespace routing, tiering, continuation matching, draft/spec storage, and eviction. Both share fundamental persistence gaps: final-name truncating writes, opaque engine state, no cryptographic integrity, no exact artifact/configuration fingerprint, no durable publication protocol, and no application-level encryption.

HaloFPX should use llama.cpp state APIs only as the **engine adapter** behind an authenticated object layer. The engine should never be asked to parse bytes until the outer cache envelope has been fully validated.
