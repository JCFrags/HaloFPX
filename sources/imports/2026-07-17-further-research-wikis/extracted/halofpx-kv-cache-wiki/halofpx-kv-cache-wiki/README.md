# HaloFPX Persistent KV-Cache Wiki

**Research cut:** 2026-07-17  
**Primary subject:** CachyLLama SSD-backed KV cache  
**Comparators:** current llama.cpp state/session and server-slot persistence, LMCache local disk, SGLang HiCacheFile, and vLLM automatic-prefix-cache keying  
**Deliverables:** source-pinned analysis, storage schemas, state diagrams, failure matrices, security and endurance models, and executable validators/fault injection

> **Mandatory integrity rule:** a cache object is a hit only after namespace authorization, bounded parsing, exact compatibility matching, complete-length checks, and cryptographic integrity verification. Every unknown, unsupported, truncated, inconsistent, corrupt, undecryptable, or engine-rejected state is **`MISS_RECOMPUTE`**. No unverified bytes may be installed into a live inference context.

## Evidence legend

| Label | Meaning |
|---|---|
| **OBSERVED** | Directly supported by source at the pinned commit. |
| **INFERENCE** | Consequence derived from observed code and documented filesystem/runtime semantics; assumptions are stated. |
| **HALOFPX RECOMMENDATION** | Proposed redesign, not behavior claimed for an audited project. |
| **NOT TESTED** | Static review only; no claim of runtime or hardware validation. |

## Principal findings

1. **CachyLLama implements real automatic SSD checkpoint reuse**, including hot/warm/cold tiers, per-conversation files, user-isolated paths, restart scans, optional draft/speculative blobs, and a separate global system-prompt cache. Its persisted v3/v1 formats are native C structs followed by opaque engine state.
2. **The observed CachyLLama write path is not transactionally published.** Checkpoint and index files are opened at their final names with truncation. There is no temporary-object/rename protocol, directory `fsync`, payload checksum, or authenticated tag. A crash can therefore leave a visible partial file; a same-length payload bit flip is not detectable by the format.
3. **The CachyLLama compatibility fingerprint is only a filter.** It is FNV-1a over `llama_model_desc()` and K/V cache types. It does not explicitly commit to model shard digests, tokenizer/chat template, adapters, runtime context/RoPE parameters, draft-model identity, or serialization ABI/build.
4. **Current llama.cpp provides manual state/session and server slot save/restore**, with strict magic/version checks and useful length validation. It is not an automatic restart-recoverable persistent prefix cache and likewise lacks atomic durable publication, checksums, encryption, catalog migration, or a complete compatibility fingerprint.
5. **SGLang HiCacheFile has stronger visibility semantics** through unique temporary files plus `os.replace`, and it rescans pages for byte/LRU accounting at startup. Its page objects remain raw, unchecksummed tensors; there is no object header, exact model artifact fingerprint, file/directory `fsync`, or process-shared catalog transaction.
6. **LMCache has strong chunk-addressing and byte-policy machinery**, but the audited local disk backend starts with an empty in-memory catalog and does not rebuild it from `.pt` files. Its local files are durable bytes, not independently restart-discoverable cache entries, and its read path does not reject short reads.
7. **vLLM provides the strongest audited prefix-key composition**, chaining a parent hash with exact block tokens and extras for multimodal inputs, LoRA name, prompt embeddings, and optional cache salt. It remains an in-memory design baseline rather than an on-disk integrity design.
8. **HaloFPX should use immutable, content-addressed objects plus generationed keyed-authenticated manifests**, a canonical full compatibility digest, per-segment SHA-256 or AEAD, durable temp-write/verify/rename/directory-sync publication, multi-process CAS/single-flight coordination, and miss-on-any-invalid-state semantics. An offline validator may report an authenticated unbound catalog entry or a fully request-bound import candidate; `HIT_VERIFIED` is reserved for a complete isolated engine import.

## Open the Wiki

- Offline rendered handbook: [`site/index.html`](site/index.html)
- GitHub Wiki entry: [`Home.md`](Home.md)
- MkDocs source entry: [`docs/index.md`](docs/index.md)
- Navigation: [`_Sidebar.md`](_Sidebar.md) or [`SUMMARY.md`](SUMMARY.md)

## Run the validation suite

```bash
cd validation
./run_all.sh
```

The suite uses only Python's standard library. It generates deterministic valid and corrupt fixtures, validates the observed CachyLLama LP64/little-endian layouts, validates the proposed HaloFPX envelope and HMAC-authenticated manifest, injects truncation and bit corruption, checks the `MISS_RECOMPUTE` contract, and produces endurance reports under `validation/results/`.

## Folder map

```text
Home.md, _Sidebar.md, _Footer.md   GitHub Wiki files
SUMMARY.md                         GitBook-style navigation
mkdocs.yml                         MkDocs navigation and styling
docs/                              Research and redesign chapters
schemas/                           Observed and proposed storage schemas
diagrams/                          Mermaid, Graphviz, and rendered SVG diagrams
tables/                            Failure, compatibility, comparison, endurance CSVs
validation/                        Executable validators, fixtures, and fault injection
tools/                             Reproducible offline-site builder
research/                          Source lock and machine-readable claim ledger
site/index.html                    Pre-rendered offline handbook
```

## Scope boundary

This is a source-level architecture and integrity review, not a benchmark, penetration test, or certification. Distributed SGLang backends such as Mooncake, 3FS, NIXL, and AIBrix were not exhaustively audited; the comparison focuses on the primary-source local file implementation and its surrounding keying/eviction logic. The explicit platform schema for CachyLLama assumes the common little-endian LP64 ABI because the implementation serializes native structs rather than a canonical wire format.
