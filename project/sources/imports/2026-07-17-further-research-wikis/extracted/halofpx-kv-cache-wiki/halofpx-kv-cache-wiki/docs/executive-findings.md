# Executive findings

## Decision summary

| System | What it provides | Integrity posture observed | Restart posture | Design conclusion |
|---|---|---|---|---|
| **CachyLLama** | Automatic SSD-backed full-context checkpoints; hot/warm/cold RAM tiers; per-conversation/user routing; system-prompt cache | Native structs, magic values, weak compatibility hash; no payload checksum/AEAD; direct final-name truncating writes | Scans checkpoint files and loads index, but index loss can reset `next_id` and collide with existing IDs | Valuable functional prototype; persistence layer needs replacement before treating a hit as trusted state |
| **llama.cpp** | Full and per-sequence state files; manual server slot save/restore; single prompt-cache file; in-memory server prompt reuse | Strict magic/version and useful size checks; no checksum, atomic publish, directory sync, full model fingerprint, or encryption | Manual restore; no general persistent-prefix catalog or eviction | Safer parser surface than CachyLLama in several places, but not the target persistent-cache architecture |
| **LMCache local disk** | Prefix-chained chunks, asynchronous disk I/O, byte cap and policy eviction | Raw bytes, direct writes, no checksum; short-read count is not checked | Local metadata mapping starts empty; no startup scan in the audited backend | Disk tier substrate, not independently restart-recoverable storage |
| **SGLang HiCacheFile** | Page-granular L3 file backend, SHA-256 parent chain, startup scan, LRU/space watermarks | Unique temp + `os.replace`; raw bytes; exact short-read check; no checksum/header/fsync/full fingerprint | Existing pages are discoverable after restart | Best observed file-publication visibility; still insufficient for authenticated durable state |
| **vLLM APC** | Parent-chained block hashes with exact tokens and multimodal/LoRA/salt extras; full-block reuse | Strong key-composition baseline, configurable SHA-256/canonical CBOR | In-memory; no on-disk object or recovery protocol | Reuse its keying principles, not as a persistence implementation |
| **HaloFPX proposal** | Content-addressed immutable objects, generationed HMAC-authenticated manifests, exact compatibility and prompt roots | Durable atomic commit, bounded parser, object/segment digests, optional AEAD, quarantine and reason codes | Rebuildable catalog; copy-on-read migration; orphan GC | Required target architecture |

## Highest-severity observed risks in CachyLLama

<span class="badge risk">HIGH</span> **Same-length corruption is not detectable.** Checkpoint payloads have no checksum or authenticated tag. A bit flip can pass file-length and magic checks and reach the engine's state importer. Whether it is rejected then depends on opaque serialization semantics rather than the cache format.

<span class="badge risk">HIGH</span> **Crash-visible partial writes.** Checkpoints and `index.bin` are opened at their final names with `O_TRUNC`. An optional file `fsync` does not make publication atomic, and the parent directory is not synchronized.

<span class="badge risk">HIGH</span> **Weak compatibility proof.** The FNV-1a fingerprint covers a model-description string and K/V cache types, not an exact artifact/configuration manifest.

<span class="badge risk">HIGH</span> **Index-loss ID reuse.** When `index.bin` is absent or invalid, initialization resets `next_id` to 1 and scans checkpoint headers, but the observed recovery path does not advance `next_id` to `max(existing_id)+1`. A later store can truncate an existing `ckpt-1.bin`.

<span class="badge risk">HIGH</span> **Unbounded trust in persisted lengths.** A checkpoint scan accepts a full header after checking its magic, then records body sizes and token count without a canonical header checksum, exact total-length check, or upper bounds. Later promotion allocates using those values.

<span class="badge risk">MEDIUM</span> **Native ABI format.** Raw C structs make the format dependent on field layout, padding, integer widths, and endianness. The supplied schemas therefore describe an LP64 little-endian interpretation, not a portable wire contract.

<span class="badge risk">MEDIUM</span> **Process-only concurrency.** Mutexes protect most in-process operations, but there is no multi-process transaction or lease protocol. Metadata pointers can escape lock scope, and some prefetch/system-cache paths have race windows.

## Required acceptance bar

A HaloFPX implementation is acceptable only when the following public-API test is true:

> For every injected truncation, header mutation, length inconsistency, model/config mismatch, prompt mismatch, payload mutation, stale generation, missing encryption key, authentication failure, and engine-import failure, the public cache API returns a typed miss, leaves the destination context unchanged, and recomputes from trusted tokens. It never returns a partial hit.

The included [validation suite](validation.md) operationalizes this rule.
