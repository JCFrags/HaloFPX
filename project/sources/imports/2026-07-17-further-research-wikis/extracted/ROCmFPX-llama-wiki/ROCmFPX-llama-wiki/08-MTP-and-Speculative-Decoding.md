# MTP and speculative decoding

> **Audit snapshot:** fork `a5605a72768c6562241b248e268e33dc92787394`; upstream `86d86ed4396b4130922f7b9af26e3d9fc11a591b`; inventory date `2026-07-17`.  
> **Decision vocabulary:** **RETAIN** = capability/ABI must survive; **REFRESH** = preserve capability but re-port onto current upstream; **RETIRE** = remove the fork patch and use upstream or archive it. [S-FORK-HEAD](https://github.com/charlie12345/ROCmFPX/tree/a5605a72768c6562241b248e268e33dc92787394) [S-UP-HEAD](https://github.com/ggml-org/llama.cpp/tree/86d86ed4396b4130922f7b9af26e3d9fc11a591b)

## Fork work inventory

| Work item | Commit/path anchor | Behavior | Decision | Primary sources |
|---|---|---|---|---|
| HY3 MTP/NextN graph | `630fa5a…`, `src/models/hyv3.cpp` | Loads appended NextN blocks, builds an MTP decoder graph, shares embeddings/head where absent, and splits target/draft memory by layer. | **RETIRE generic implementation** | [S-C-630](https://github.com/charlie12345/ROCmFPX/commit/630fa5a0f8fc04689b86d1b0a3d75b2b7d546d07) [S-HYV3-FORK](https://github.com/charlie12345/ROCmFPX/blob/a5605a72768c6562241b248e268e33dc92787394/src/models/hyv3.cpp) [S-UP-PR25395](https://github.com/ggml-org/llama.cpp/pull/25395) |
| Native MTP hardening | `eff9987…`, `common/speculative.cpp` | Tightens context/limit behavior and effective draft depth handling. | **REFRESH** | [S-C-EFF](https://github.com/charlie12345/ROCmFPX/commit/eff9987923b58d1a6b7e54610c667803ac2d0ea7) [S-SPEC-FORK](https://github.com/charlie12345/ROCmFPX/blob/a5605a72768c6562241b248e268e33dc92787394/common/speculative.cpp) [S-SPEC-UP](https://github.com/ggml-org/llama.cpp/blob/86d86ed4396b4130922f7b9af26e3d9fc11a591b/common/speculative.cpp) |
| Strict greedy HY3 verification | `7d7b06b…` | Verifies the target in a strict path intended to preserve greedy token output. | **RETAIN** | [S-C-7D7](https://github.com/charlie12345/ROCmFPX/commit/7d7b06bc5e6fd4a128af8efa954caa7409376a79) |
| One-slot strict policy | `2766f41…` | Changes automatic server parallelism for strict HY3. | **REFRESH** | [S-C-276](https://github.com/charlie12345/ROCmFPX/commit/2766f419526ea14ba1be8f31eca21263cfc52813) |
| Per-request limits | `b56ad79…`, server task/context | Applies `n_min`, `n_max`, and `p_min` overrides per request. | **RETAIN** | [S-C-B56](https://github.com/charlie12345/ROCmFPX/commit/b56ad79d77bc1eb9fe6407640eec8b8edfc04900) [S-SERVER-TASK](https://github.com/charlie12345/ROCmFPX/blob/a5605a72768c6562241b248e268e33dc92787394/tools/server/server-task.cpp) |
| Stateful SSD prompt cache | `c81c7c9…`, server/speculative files | Stores target, draft, and implementation-specific speculative state and restores them together. | **REFRESH** | [S-C-C81](https://github.com/charlie12345/ROCmFPX/commit/c81c7c92233b6370b4eb7087398779a8dcb234a4) [S-SERVER-CTX](https://github.com/charlie12345/ROCmFPX/blob/a5605a72768c6562241b248e268e33dc92787394/tools/server/server-context.cpp) [S-SERVER-TEST](https://github.com/charlie12345/ROCmFPX/blob/a5605a72768c6562241b248e268e33dc92787394/tools/server/tests/unit/test_prompt_cache_disk.py) |
| Portable cache sync/paths/tests | `bb7d9cb…`, `756121a…`, `0d7ac51…` | Adds portable flush/sync, UTF-8 paths, and failure probes. | **REFRESH/RETAIN tests** | [S-C-BB7](https://github.com/charlie12345/ROCmFPX/commit/bb7d9cb5965e3be1ce2073134ba14787bf378113) [S-C-756](https://github.com/charlie12345/ROCmFPX/commit/756121a5e8e7da464aebd2ab344a2aefef6cecac) [S-C-0D7](https://github.com/charlie12345/ROCmFPX/commit/0d7ac512e5eb511843797c07a16bc7d97d3bc4f8) |
| HY3 MTP converter/export | `f961404…`, converter | Exports MTP tensors/splits. | **RETIRE** | [S-C-F961](https://github.com/charlie12345/ROCmFPX/commit/f961404519a2ed286b750ba1419d40318a6b9a92) [S-UP-SPLIT-MTP](https://github.com/ggml-org/llama.cpp/commit/cb489bc0fb789c2cb7a9cc9dc44fa71893fe0988) |

## State that the cache must treat atomically

The fork’s speculative implementation carries implementation-private boundary state in addition to the normal target and draft llama contexts. Commit `c81c7c9…` extends state APIs and tracks current/previous pending hidden-state boundaries, verification rows/positions, and position shifts so a restored or shifted cache remains coherent. [S-C-C81](https://github.com/charlie12345/ROCmFPX/commit/c81c7c92233b6370b4eb7087398779a8dcb234a4) [S-SPEC-FORK](https://github.com/charlie12345/ROCmFPX/blob/a5605a72768c6562241b248e268e33dc92787394/common/speculative.cpp)

A valid cache entry therefore has at least three coupled payloads:

1. target context state;
2. draft/MTP context state;
3. speculative implementation state and version.

Restoring only the target KV state can produce a semantically inconsistent draft boundary. [S-C-C81](https://github.com/charlie12345/ROCmFPX/commit/c81c7c92233b6370b4eb7087398779a8dcb234a4) [S-SERVER-CTX](https://github.com/charlie12345/ROCmFPX/blob/a5605a72768c6562241b248e268e33dc92787394/tools/server/server-context.cpp) [S-SPEC-FORK](https://github.com/charlie12345/ROCmFPX/blob/a5605a72768c6562241b248e268e33dc92787394/common/speculative.cpp)

## Upstream base to adopt

The pinned upstream `common/speculative.cpp` is newer than the fork copy and is integrated with current model/private context APIs. Upstream HY3/MTP should own model loading, graph construction, draft-context creation, and ordinary accept/rollback behavior. [S-SPEC-UP](https://github.com/ggml-org/llama.cpp/blob/86d86ed4396b4130922f7b9af26e3d9fc11a591b/common/speculative.cpp) [S-HYV3-UP](https://github.com/ggml-org/llama.cpp/blob/86d86ed4396b4130922f7b9af26e3d9fc11a591b/src/models/hy-v3.cpp) [S-EXT-UP](https://github.com/ggml-org/llama.cpp/blob/86d86ed4396b4130922f7b9af26e3d9fc11a591b/src/llama-ext.h) [S-UP-PR25395](https://github.com/ggml-org/llama.cpp/pull/25395)

## Minimal fork delta to re-port

### Strict verification

Preserve strict greedy verification as a separate mode with a deterministic invariant: for a fixed model, prompt, context, and greedy sampling configuration, strict-MTP output must match non-speculative target output token-for-token. The fork commit is the primary behavior source; the migration must add fresh equivalence tests around the upstream engine. [S-C-7D7](https://github.com/charlie12345/ROCmFPX/commit/7d7b06bc5e6fd4a128af8efa954caa7409376a79) [S-SPEC-UP](https://github.com/ggml-org/llama.cpp/blob/86d86ed4396b4130922f7b9af26e3d9fc11a591b/common/speculative.cpp)

### Request policy

Preserve per-request `n_min`, `n_max`, and `p_min`, but implement them through the current server schema/task model. The pinned upstream schema contains speculative request fields in disabled code, so the fork feature is not yet replaceable by an enabled upstream request contract. [S-C-B56](https://github.com/charlie12345/ROCmFPX/commit/b56ad79d77bc1eb9fe6407640eec8b8edfc04900) [S-SERVER-TASK](https://github.com/charlie12345/ROCmFPX/blob/a5605a72768c6562241b248e268e33dc92787394/tools/server/server-task.cpp) [S-SERVER-SCHEMA-UP](https://github.com/ggml-org/llama.cpp/blob/86d86ed4396b4130922f7b9af26e3d9fc11a591b/tools/server/server-schema.cpp)

### Stateful disk cache

Preserve the ability to cache stateful speculative sessions, but refresh the implementation with:

- an explicit cache format version;
- model and architecture fingerprint;
- context/configuration fingerprint;
- target/draft/spec-state length and checksum;
- atomic write/rename and durable sync policy;
- owner-only permissions and path traversal protection;
- graceful rejection of old/incomplete entries;
- restore equivalence tests across process restart.

The existing fork code and tests establish the capability and portability issues; the hardening list is an engineering requirement for a maintainable replacement. [S-C-C81](https://github.com/charlie12345/ROCmFPX/commit/c81c7c92233b6370b4eb7087398779a8dcb234a4) [S-C-BB7](https://github.com/charlie12345/ROCmFPX/commit/bb7d9cb5965e3be1ce2073134ba14787bf378113) [S-C-756](https://github.com/charlie12345/ROCmFPX/commit/756121a5e8e7da464aebd2ab344a2aefef6cecac) [S-C-0D7](https://github.com/charlie12345/ROCmFPX/commit/0d7ac512e5eb511843797c07a16bc7d97d3bc4f8) [S-SERVER-TEST](https://github.com/charlie12345/ROCmFPX/blob/a5605a72768c6562241b248e268e33dc92787394/tools/server/tests/unit/test_prompt_cache_disk.py)

## One-slot policy

Automatically forcing one slot can avoid a known correctness boundary, but it silently changes capacity. The refreshed implementation should either prove multi-slot correctness or fail/warn explicitly when strict mode and parallelism conflict. Preserve the constraint, not necessarily the hidden auto-override. [S-C-276](https://github.com/charlie12345/ROCmFPX/commit/2766f419526ea14ba1be8f31eca21263cfc52813) [S-C-7D7](https://github.com/charlie12345/ROCmFPX/commit/7d7b06bc5e6fd4a128af8efa954caa7409376a79)

## Acceptance matrix

| Test | Non-negotiable assertion | Primary sources |
|---|---|---|
| Greedy equivalence | strict MTP token stream equals target-only token stream. | [S-C-7D7](https://github.com/charlie12345/ROCmFPX/commit/7d7b06bc5e6fd4a128af8efa954caa7409376a79) |
| Cache round trip | uninterrupted result equals save/restart/restore result. | [S-C-C81](https://github.com/charlie12345/ROCmFPX/commit/c81c7c92233b6370b4eb7087398779a8dcb234a4) [S-SERVER-TEST](https://github.com/charlie12345/ROCmFPX/blob/a5605a72768c6562241b248e268e33dc92787394/tools/server/tests/unit/test_prompt_cache_disk.py) |
| Position shift | shifted/trimmed contexts retain correct private speculative boundary positions. | [S-C-C81](https://github.com/charlie12345/ROCmFPX/commit/c81c7c92233b6370b4eb7087398779a8dcb234a4) |
| Request isolation | per-request overrides do not leak to other slots or defaults. | [S-C-B56](https://github.com/charlie12345/ROCmFPX/commit/b56ad79d77bc1eb9fe6407640eec8b8edfc04900) |
| Multi-context RPC | switching target/draft contexts on one RPC device does not reuse a stale graph UID. | [S-RPC-MTP-FIX](https://github.com/ggml-org/llama.cpp/commit/c3e9ade6dd3ff2a1ceafd2d59062634715b472c4) |
| HY3 split model | target and MTP side/split files resolve shared tensors and layer offsets correctly. | [S-UP-PR25395](https://github.com/ggml-org/llama.cpp/pull/25395) [S-UP-SPLIT-MTP](https://github.com/ggml-org/llama.cpp/commit/cb489bc0fb789c2cb7a9cc9dc44fa71893fe0988) |
