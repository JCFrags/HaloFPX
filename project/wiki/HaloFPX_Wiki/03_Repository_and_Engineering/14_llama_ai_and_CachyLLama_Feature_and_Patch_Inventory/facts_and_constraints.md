---
section_id: "14"
title: "Facts and Constraints"
status: "needs-machine-validation"
last_verified: "2026-07-16"
applies_to:
  repositories:
    - "fewtarius/llama-ai@1017f3dfdce3ca2b06aa9007b23295db3bb35722"
    - "fewtarius/CachyLLama@6be745998f568e379ea197fcf827baec73ff9940"
    - "ggml-org/llama.cpp@92366df30d4eaa4b85139b5fd694360237731b19"
  software_versions: []
  hardware_revisions: []
related_sections: ["11", "12", "13", "15", "16"]
---

# Facts and Constraints

## Patch-surface inventory

**[VERIFIED]** Comparing CachyLLama `6be7459` to its merged upstream parent `92366df` yields 56 changed paths. This is the relevant frozen patch inventory, not a comparison with a later moving upstream branch. [S14-002][S14-003]

| Capability | Primary fork paths | Observed behavior | Proof boundary |
|---|---|---|---|
| checkpoint persistence | `common/kv-ssd-cache.{h,cpp}`, `tools/server/server-context-ssd-cache.*` | v3 per-checkpoint files store target, optional draft/MTP, and speculative-state blobs | code inspected; no dedicated end-to-end corruption suite found |
| hot/warm/cold tiers | `common/kv-ssd-cache.cpp` | hot and warm are RAM maps; cold is SSD-only; inactivity and RAM budgets demote/promote entries | page-manager unit program covers tier primitives, not server correctness |
| system-prompt cache | `common/kv-ssd-system-cache.*`, `tools/server/server-context.cpp` | cross-conversation entries, default 8 entries and 30 unused days; prompt boundary detected from tokens/template | boundary/model matrix not comprehensively tested |
| continuation matching | `kv_ssd_find_match`, `kv_ssd_find_continuation`, page manager | longest common prefix (LCP), model compatibility hash, and a cross-conversation overlap threshold | only first 4,096 tokens are retained for matching |
| user isolation | `server-task.*`, `server-context-page-manager.*`, `server-context.cpp` | validated `llama_user_id`, hashed `u/` namespace, cross-user lookup disabled, slot affinity, explicit-user cap | not authentication; anonymous-cap docs contradict implementation |
| recurrent/hybrid restore | `src/llama-memory-hybrid.*`, `server-context.cpp` | attention-only removal preserves recurrent state; low-coverage hybrid checkpoints are rejected/capped | comments acknowledge recurrent content sensitivity; requires model tests |
| MTP/speculative restore | cache v3 blobs and `server-context.cpp` | draft context plus speculative implementation state (`pending_h`) can be persisted/restored | exact ABI is runtime- and commit-coupled |
| expert telemetry | `include/llama.h`, `src/llama-context.*`, server routes | opt-in per-layer activation counts; GET/POST endpoints; no compute-placement change | no telemetry correctness/load test found |
| runner/profiles | `llama-run.sh` | filename/GGUF-based selection: `ssm-optimized`, `moe-optimized`, `large-dense`, `medium-dense`, `small-efficient` | policy, not portable truth |
| benchmarks | `scripts/benchmark.sh`, `benchmarks/` | launches cold and warm server phases and records responses, stats, logs, and summaries | repository-authored single-host measurements |

## Persistent checkpoint format and durability

**[VERIFIED]** Cache format v3 uses `index.bin` (`KVID`) plus `ckpt-{id}.bin` (`KVRC`). Each record contains IDs, positions, token count, turn, model compatibility hash, up to 4,096 prefix tokens, and byte lengths for target, draft/MTP, and speculative blobs. [S14-002]

**[VERIFIED]** Default low-level configuration is 2 GiB hot RAM, 1 GiB warm RAM, two/four-turn demotion, a 4,096-token hot window, 32 cold checkpoints, 15% reserve, and `fsync` enabled. `llama-ai` overrides many defaults by selected profile. [S14-001][S14-002]

**[VERIFIED]** Writes use `O_TRUNC` directly on the final checkpoint and index paths. Failed payload writes unlink the checkpoint, but there is no temporary-file-plus-atomic-rename transaction and no checksum in the record. Load checks magic/ID, reads declared lengths, and rejects a compatibility-hash mismatch. [S14-002]

**[INFERENCE]** A power loss can leave a valid-looking header with corrupted payload or a truncated/replaced index. The current checks often turn truncation into a miss, but silent bit corruption is not detected. HaloFPX cannot treat this format as trusted durable state without stronger integrity and atomicity.

**[VERIFIED]** `--cache-ssd-no-fsync` opts out of checkpoint/index synchronization and explicitly warns that the last checkpoint may be lost on crash. The system-prompt cache always calls `fsync`, but likewise truncates the final path directly. [S14-002]

## Matching and state acceptance

**[VERIFIED]** Within a conversation, the cache selects the compatible checkpoint with greatest LCP, then newest turn and largest token count. It accepts any non-zero LCP at this layer and leaves safety validation to the server. [S14-002]

**[VERIFIED]** Cross-conversation continuation scans cache directories, reads stored prefixes, requires at least 16 comparable tokens, scores `matches / n_tokens`, and filters by compatibility hash and caller threshold. User-scoped traffic routes to `u/{FNV1a(user_id)}` and does not use fuzzy cross-user lookup. [S14-002][S14-005]

**[VERIFIED]** Hybrid restore uses three cases: full coverage; a 4,096-token prefix plus at least 99% reported overlap; or partial coverage capped to the LCP. Below 80% of the validated prefix it rejects and clears the loaded state. Attention-only clearing is used to preserve recurrent state. [S14-002]

**[VERIFIED]** The compatibility hash is described in logs/code as architecture dimensions plus K/V cache types. It is not a cryptographic identity of exact model bytes, tokenizer, chat template, runtime commit, backend, or serialized-state ABI. [S14-002]

## System-prompt cache

**[VERIFIED]** The server detects a system boundary from tokenized input and chat template, ignores sections shorter than 16 tokens, saves partial sequence state at that boundary, and keys the entry from the system token sequence. Entries are global within the model cache directory and LRU/age limited. [S14-002]

**[VERIFIED]** Cross-conversation reuse intentionally shares state. `llama_user_id` isolates conversation caches, but the inspected system-cache initialization is model-global rather than user-namespaced. **[OPEN]** Determine whether a system prompt can contain tenant secrets; if yes, global sharing is unacceptable without a scope key.

## User isolation, affinity, and concurrency

**[VERIFIED]** `llama_user_id` permits only `[A-Za-z0-9_-]` and a 512-character maximum. It is an operator-provided label, not an authenticated principal. [S14-002][S14-005]

**[VERIFIED]** Slot selection prefers a reusable slot owned by the same non-empty user, excludes differently owned slots for explicit users, then applies LRU. The cache directory uses a non-cryptographic FNV-1a hash of the label. [S14-002][S14-005]

**[VERIFIED]** `--max-concurrent-per-user` defaults to zero (unlimited). Explicit user counts are incremented and can return HTTP 429. [S14-002]

**[VERIFIED]** There is an internal contradiction: the design document and one allocator comment say the cap applies to `_anonymous`, but accounting increments only non-empty IDs and the HTTP fast path explicitly says anonymous requests use only global `n_parallel`. [S14-002][S14-005]

**[VERIFIED]** The HTTP handler calls `post_tasks(std::move(tasks))` before its advertised fast-path 429 loop reads `tasks`. **[INFERENCE]** Because a moved-from vector has valid but unspecified contents, this fast path cannot be relied upon to inspect the submitted tasks; the later slot allocator can refuse an over-cap task, but that is not proof the client receives the documented immediate 429. [S14-002]

**[VERIFIED]** `server_slot::release()` calls `reset()` before `callback_on_release`; `reset()` clears `user_id_`, while the callback decrements the counter only when `user_id_` is non-empty. The prompt-similarity selection loop also does not filter by user, and the prompt tokens can survive a normal non-child release. [S14-002]

**[INFERENCE]** At this pin, per-user counts can remain charged after completion, advertised affinity ownership is erased, and a later user's similar prompt can select another user's residual in-memory slot state. These are blocking lifecycle and isolation risks; the feature must not be adopted without a reproducing test and repair.

## Runner profiles and server launch

**[VERIFIED]** `llama-run.sh` combines filename heuristics with a GGUF-header scan. Pure SSM disables context shift, checkpoints, and SSD caching; MoE forces one parallel slot; dense models get size/tier-specific cache budgets. Several Halo profiles deliberately disable SSD cache to protect the 96 GiB memory envelope. [S14-001]

**[VERIFIED]** The runner exposes `--no-ssd-cache`, cache budgets, checkpoint controls, `-np`, reasoning preservation/budget, backends, and profile printing. It launches metrics but does not expose the CachyLLama per-user cap or expert-tracking controls in its own documented profile interface. [S14-001]

**[VERIFIED]** `systemd/llama-server.service` still uses `--checkpoint-every-n-tokens`, while current runner parsing accepts `--checkpoint-min-step`. The unit also hard-codes `/home/deck/llama-ai` and a model name. It is a stale example, not a deployable HaloFPX unit. [S14-001]

## Tests and measurements

**[VERIFIED]** The pinned `llama-ai` repository carries GPL-3.0, while the pinned CachyLLama/llama.cpp tree carries MIT. Selective reuse must preserve per-file provenance and be reviewed with the complete dependency and documentation licenses in section 16. [S14-007]

**[VERIFIED]** `test_kv_page_manager.cpp` defines 12 standalone tests for allocation, basic get/put, cold load, prefetch, dirty flush, eviction, tiers, auto-size, config, stats, and page sizes. It is placed at repository root and was not found registered in the inspected CMake test manifests. [S14-002]

**[VERIFIED]** Upstream-derived tests include save/load, fragmented restore, recurrent rollback, and server slot tests, but no pinned test directly establishes crash atomicity, checksum rejection, cross-user noninterference, MTP cold-start equivalence, or two-node rank-local behavior. [S14-002]

This section records no HaloFPX measurement. Checked-in `llama-ai` benchmark data is evidence produced on the repository author's machines, not a measurement made by this project. [S14-004]
