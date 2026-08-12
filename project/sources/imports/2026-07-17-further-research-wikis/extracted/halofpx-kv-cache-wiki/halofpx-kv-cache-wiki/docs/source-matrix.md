# Source matrix

All implementation claims are tied to these exact primary-source commits. URLs are immutable commit links.

## CachyLLama (`6be745998f568e379ea197fcf827baec73ff9940`)

<a id="c-kv-h"></a>
| ID | Path | Evidence used |
|---|---|---|
| C-KV-H | [`common/kv-ssd-cache.h`](https://github.com/fewtarius/CachyLLama/blob/6be745998f568e379ea197fcf827baec73ff9940/common/kv-ssd-cache.h) | Config defaults, native index/record structs, public API, namespaces |
| C-KV-CPP | [`common/kv-ssd-cache.cpp`](https://github.com/fewtarius/CachyLLama/blob/6be745998f568e379ea197fcf827baec73ff9940/common/kv-ssd-cache.cpp) | FNV, writes/reads, index, scan, tiers, eviction, continuation, prefetch |
| C-SYS-H | [`common/kv-ssd-system-cache.h`](https://github.com/fewtarius/CachyLLama/blob/6be745998f568e379ea197fcf827baec73ff9940/common/kv-ssd-system-cache.h) | System v1 format and retention contract |
| C-SYS-CPP | [`common/kv-ssd-system-cache.cpp`](https://github.com/fewtarius/CachyLLama/blob/6be745998f568e379ea197fcf827baec73ff9940/common/kv-ssd-system-cache.cpp) | System scan/load/store/eviction and error behavior |
| C-PM | [`tools/server/server-context-page-manager.cpp`](https://github.com/fewtarius/CachyLLama/blob/6be745998f568e379ea197fcf827baec73ff9940/tools/server/server-context-page-manager.cpp) | Compatibility hash, routing, directory/user eviction, load fallback |
| C-PM-H | [`tools/server/server-context-page-manager.h`](https://github.com/fewtarius/CachyLLama/blob/6be745998f568e379ea197fcf827baec73ff9940/tools/server/server-context-page-manager.h) | Manager state and locking model |
| C-SRV | [`tools/server/server-context-ssd-cache.cpp`](https://github.com/fewtarius/CachyLLama/blob/6be745998f568e379ea197fcf827baec73ff9940/tools/server/server-context-ssd-cache.cpp) | Target/draft/spec serialization and restore policy |
| C-USER | [`docs/development/user-isolation-design.md`](https://github.com/fewtarius/CachyLLama/blob/6be745998f568e379ea197fcf827baec73ff9940/docs/development/user-isolation-design.md) | Conversation/user identity and isolation intent |
| C-README | [`README.md`](https://github.com/fewtarius/CachyLLama/blob/6be745998f568e379ea197fcf827baec73ff9940/README.md) | Feature intent and CLI context, cross-checked against code |

## llama.cpp (`86d86ed4396b4130922f7b9af26e3d9fc11a591b`)

| ID | Path | Evidence used |
|---|---|---|
| L-H | [`include/llama.h`](https://github.com/ggml-org/llama.cpp/blob/86d86ed4396b4130922f7b9af26e3d9fc11a591b/include/llama.h) | State APIs, magic and version constants |
| L-CTX | [`src/llama-context.cpp`](https://github.com/ggml-org/llama.cpp/blob/86d86ed4396b4130922f7b9af26e3d9fc11a591b/src/llama-context.cpp) | Session/sequence file encoding, validation, architecture check |
| L-MMAP | [`src/llama-mmap.cpp`](https://github.com/ggml-org/llama.cpp/blob/86d86ed4396b4130922f7b9af26e3d9fc11a591b/src/llama-mmap.cpp) | `llama_file` open/write/close behavior |
| L-SRV | [`tools/server/server-context.cpp`](https://github.com/ggml-org/llama.cpp/blob/86d86ed4396b4130922f7b9af26e3d9fc11a591b/tools/server/server-context.cpp) | In-memory prompt cache and manual slot save/restore behavior |
| L-README | [`tools/server/README.md`](https://github.com/ggml-org/llama.cpp/blob/86d86ed4396b4130922f7b9af26e3d9fc11a591b/tools/server/README.md) | Server options and slot endpoint contract |

## LMCache (`c9439c6535503c9e17fe236da9bc88807b58c2bc`)

| ID | Path | Evidence used |
|---|---|---|
| M-DISK | [`lmcache/v1/storage_backend/local_disk_backend.py`](https://github.com/LMCache/LMCache/blob/c9439c6535503c9e17fe236da9bc88807b58c2bc/lmcache/v1/storage_backend/local_disk_backend.py) | File naming, metadata map, I/O, eviction and restart behavior |
| M-UTIL | [`lmcache/utils.py`](https://github.com/LMCache/LMCache/blob/c9439c6535503c9e17fe236da9bc88807b58c2bc/lmcache/utils.py) | CacheEngineKey and DiskCacheMetadata |
| M-TOK | [`lmcache/v1/token_database.py`](https://github.com/LMCache/LMCache/blob/c9439c6535503c9e17fe236da9bc88807b58c2bc/lmcache/v1/token_database.py) | Prefix-chained token hash and extra-key behavior |

## SGLang (`fec613184480bd6fc5bfc9967bfb24a6125f684c`)

| ID | Path | Evidence used |
|---|---|---|
| S-STOR | [`python/sglang/srt/mem_cache/hicache_storage.py`](https://github.com/sgl-project/sglang/blob/fec613184480bd6fc5bfc9967bfb24a6125f684c/python/sglang/srt/mem_cache/hicache_storage.py) | File suffix, raw object, temp/replace, reads and existence logic |
| S-LRU | [`python/sglang/srt/mem_cache/storage/file/lru_file_evictor.py`](https://github.com/sgl-project/sglang/blob/fec613184480bd6fc5bfc9967bfb24a6125f684c/python/sglang/srt/mem_cache/storage/file/lru_file_evictor.py) | Startup scan, byte/free-space eviction and reservations |
| S-UTIL | [`python/sglang/srt/mem_cache/utils.py`](https://github.com/sgl-project/sglang/blob/fec613184480bd6fc5bfc9967bfb24a6125f684c/python/sglang/srt/mem_cache/utils.py) | Parent-chained page hash calls |
| S-HASH | [`python/sglang/srt/mem_cache/cpp_utils/hash_binding.cpp`](https://github.com/sgl-project/sglang/blob/fec613184480bd6fc5bfc9967bfb24a6125f684c/python/sglang/srt/mem_cache/cpp_utils/hash_binding.cpp) | SHA-256 page hashing details |
| S-RADIX | [`python/sglang/srt/mem_cache/hiradix_cache.py`](https://github.com/sgl-project/sglang/blob/fec613184480bd6fc5bfc9967bfb24a6125f684c/python/sglang/srt/mem_cache/hiradix_cache.py) | L1/L2/L3 workflow and operation coordination |
| S-DESIGN | [`docs_new/docs/advanced_features/hicache_design.mdx`](https://github.com/sgl-project/sglang/blob/fec613184480bd6fc5bfc9967bfb24a6125f684c/docs_new/docs/advanced_features/hicache_design.mdx) | Official architecture and backend classification |

## vLLM (`bf578e1abdffc2d25232783ff59a3132279e6bdd`)

| ID | Path | Evidence used |
|---|---|---|
| V-DESIGN | [`docs/design/prefix_caching.md`](https://github.com/vllm-project/vllm/blob/bf578e1abdffc2d25232783ff59a3132279e6bdd/docs/design/prefix_caching.md) | Block-key components, full-block policy, algorithms and cache salt |
| V-KV | [`vllm/v1/core/kv_cache_utils.py`](https://github.com/vllm-project/vllm/blob/bf578e1abdffc2d25232783ff59a3132279e6bdd/vllm/v1/core/kv_cache_utils.py) | Hash root, exact block tokens, multimodal/LoRA/prompt-embed/salt extras |

## Machine-readable lock

See [`research/source-lock.json`](../research/source-lock.json) for repository, branch, commit and source URL records. The artifact does not redistribute upstream source code; it provides analysis, small independently written schemas and validators.
