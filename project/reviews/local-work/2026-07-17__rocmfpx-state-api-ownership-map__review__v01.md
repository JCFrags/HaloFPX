---
title: ROCmFPX state and API ownership map
date: 2026-07-17
status: complete-static-review
scope: preserved ROCmFPX Git objects only
runtime_qualified: false
---

# ROCmFPX state/API ownership map

## Decision

ROCmFPX already owns the authoritative model-memory codecs and the server's live generation state. A persistent cache must integrate through those target-native APIs and must not make the donor page manager, RPC tensor cache, or a new ad hoc blob format authoritative. This map establishes static ownership and fixture requirements only; it does not claim runtime qualification.

Inspection used candidate `61f2f2d7bc4955e9bca821095ef69125837133b5` and planning baseline `a5605a72768c6562241b248e268e33dc92787394`. Every state/cache/RPC path listed below has the same Git blob at both commits, so the map does not depend on the candidate-only build changes.

This record implements the static-ownership work required by the [accepted fork plan](../plans/2026-07-17__rocmfpx-llama-ai-fork-plan__draft__v03.md) and its [acceptance review](../plans/2026-07-17__rocmfpx-llama-ai-fork-plan__review__v03.md). The [Agent Harness routing note](../../references/agent-harness.md) is retained as the boundary for later harness integration; it does not supersede ROCmFPX source ownership.

## Ownership map

| Domain | Current owner and API boundary | Current cache coverage | Compatibility fixture requirement | OPEN gap |
|---|---|---|---|---|
| Whole-session state | Public `llama_state_get_size`, `llama_state_get_data`, `llama_state_set_data`, `llama_state_save_file`, and `llama_state_load_file` in `include/llama.h`; serialization in `src/llama-context.cpp`. Session format version is 9. | Not the server prompt-cache payload boundary. | Round-trip at an exact pinned model/build; corrupt magic/version/architecture must reject without usable partial state. | Whole-state identity checks architecture, with a source TODO for stronger model identity. It is insufficient as a persistent compatibility manifest. |
| Per-sequence state | `llama_state_seq_get_size`, `llama_state_seq_get_data`, `llama_state_seq_set_data`, file variants, `_ext` flags, and storage APIs in `include/llama.h`; codec in `src/llama-context.cpp`. Sequence format version is 2; internal magic is `0xaf143cd8`. | Target and draft sequence payloads are used by prompt checkpoints. | Same-slot and cross-slot restore, exact token boundary, flag matrix, truncated/corrupt blob, and destination rollback/recompute on failure. | The sequence blob does not itself carry a complete durable compatibility manifest. |
| Transformer attention | `llama_memory_i::state_write/state_read` in `src/llama-memory.h`; transformer KV implementation in `src/llama-kv-cache.cpp`. | Included in sequence state unless architecture/flags select otherwise. | Save, mutate, restore, and compare next-token logits/tokens on a transformer model; reject invalid sizes/positions. | Runtime equivalence and device/host transfer behavior remain unqualified. |
| ISWA and DSA attention variants | State implementations in `src/llama-kv-cache-iswa.cpp` and `src/llama-kv-cache-dsa.cpp`. | Included through the selected model-memory implementation. | Architecture-specific round-trip with wrapped positions and partial/full flags; compare next-token behavior, not only byte counts. | No validated fixture/model inventory is recorded yet. |
| Recurrent state | `src/llama-memory-recurrent.cpp` owns recurrent state metadata/data. | Included through sequence state. | Recurrent-only save/restore across a nontrivial boundary; mismatch must miss/recompute. | Exact supported recurrent architecture matrix and boundary invariants need qualification. |
| Hybrid state | `src/llama-memory-hybrid.cpp` owns attention plus recurrent composition; without `LLAMA_STATE_SEQ_FLAGS_PARTIAL_ONLY` it includes attention and always includes recurrent. `src/llama-memory-hybrid-iswa.cpp` adds ISWA/recurrent/DSV4 composition. | Included through sequence state when the correct flags and model implementation are used. | Full vs partial-only matrix; attention/recurrent positions must agree; include DSV4/ISWA variants. | Flag selection policy for durable cache entries must be specified and versioned. |
| State location flags | `LLAMA_STATE_SEQ_FLAGS_NONE`, `LLAMA_STATE_SEQ_FLAGS_PARTIAL_ONLY`/`SWA_ONLY`, and `LLAMA_STATE_SEQ_FLAGS_ON_DEVICE` in `include/llama.h`; storage clone/init/free/size and get/set storage APIs. | ROCmFPX prompt checkpoints can retain storage handles; disk uses sequence files. | Host vs on-device state parity, storage clone lifetime, failed set cleanup, and memory-pressure fallback. | A durable provider must define whether device storage is a hot tier only and how it degrades safely. |
| Target/draft prompt checkpoint | `common_prompt_checkpoint` in `common/common.h` and `common/common.cpp` owns target/draft bytes or storage handles, token/position metadata, update, and load. | Used by server prompt cache for target and optional draft state. | Atomically restore target and draft to the same boundary; missing, stale, or incompatible draft must reject the whole entry. | Persistent metadata schema and cryptographic content identity are absent. |
| Speculative aggregate | `common/speculative.h` and `common/speculative.cpp` own typed implementation state APIs: get, set, and required. | `server_context` requires speculative state for save/load when the selected implementation says it is required. | Required/unavailable state must block save; missing/wrong type must block load and leave the slot clean. | Aggregate get currently documents support for only one implementation state. Multi-implementation behavior needs an explicit contract. |
| MTP state | MTP speculative implementation in `common/speculative.cpp` owns pending current/previous hidden state, positions, and flags. | Kept with the in-memory prompt-cache entry metadata; it is not independently durable across restart. | Restore current and previous pending hidden state at the exact target/draft boundary; wrong dimensions/positions/type must miss. | Durable encoding, versioning, and restart recovery for MTP metadata are absent. |
| Eagle state | Eagle speculative implementation in `common/speculative.cpp` implements typed state get/set/required. | Same server save/load gate as other required speculative implementations. | Eagle-specific exact-boundary round-trip and incompatible-type rejection. | Exact durable representation and version compatibility remain undefined. |
| Sampler, RNG, grammar, reasoning | `common/sampling.h` and `common/sampling.cpp` own the sampler chain, grammar, reasoning budget, prior tokens, candidates, clone, reset, and accept behavior. Server slots initialize/reset and replay prompt tokens. | Not serialized by the sequence state API or current disk prompt-cache files. | Prove replay reconstructs sampler, RNG, grammar lazy triggers, reasoning budget, and prior-token state exactly; otherwise add a versioned target-native codec. Seed equality alone is not sufficient evidence. | There is no serialize-to-bytes API for the complete sampler/grammar/RNG state. This blocks transparent arbitrary mid-generation persistence. |
| Live server slot | `tools/server/server-context.cpp` owns target/draft contexts, speculative instance and draft buffers, prompt checkpoint, sampler, prompt/generated tokens, task, and generation counters. | Prompt checkpoint covers only the model/spec subset required for prompt reuse. | Restore must occur while a slot is quiescent; test cancellation, slot reuse, cross-slot destination ID, and cleanup after partial failure. | Quiesce/lease protocol and ownership transfer to a background cache provider are not specified. |
| Request and streaming-result state | `tools/server/server-task.h` and `tools/server/server-task.cpp` own request parameters/tokens and result streaming state, including chat/tool diffs, response identifiers, and thinking/text block state. | Not part of model prompt-cache persistence. | Cancellation and retry matrix before first partial, after partial, before final, and after final queueing. Verify no duplicated committed output. | No durable request/result recovery contract exists; it must remain separate from prompt-state reuse unless explicitly designed. |
| Observable output boundary | `tools/server/server-context.cpp` owns partial/final/error queueing; `tools/server/server-http.cpp` turns queued results into HTTP/SSE output. | Not persisted by prompt cache. | Define and test the commit boundary and delivery semantics under disconnect, retry, cancellation, and process failure. | There is no explicit durable output transaction. Final-result queueing is only a commit-like observable boundary, not proof of exactly-once delivery. |
| Current RAM/disk prompt cache | `server_prompt_cache` in `tools/server/server-task.h` and `tools/server/server-task.cpp`: RAM states, owner-only per-run directory, advisory lock, stale cleanup, temp write, exact-size check, fsync, rename, directory fsync, circuit breaker, bounded LRU, and LCP selection. | Target/draft files can spill to disk during a run. Destructor renames/removes the run directory; speculative bytes live in entry metadata. | Crash at every transaction stage; corrupt/truncate/replace payload; stale lock/run cleanup; LRU and circuit breaker; ensure every incompatibility becomes miss/recompute and leaves destination clean. | Explicitly ephemeral. No restart catalog, durable speculative metadata, tenant scope, or complete compatibility identity. |
| Future persistent provider/catalog | No current authoritative implementation. It must sit behind a target-owned interface and call target state APIs. | None. | Catalog replay, atomic publish, hash verification, unknown-version quarantine, corruption-as-miss, quota/eviction, tenant isolation, restart, and single-node fallback. | Schema, identity tuple, lock/lease model, directory layout, migration, observability, and deletion policy are design gates. |
| RPC model/tensor transport | Backend registration in `common/arg.cpp`; public RPC boundary in `ggml/include/ggml-rpc.h`; transport/tensor/graph commands and tensor hash cache in `ggml/src/ggml-rpc/ggml-rpc.cpp`. | Separate model-tensor cache; it does not own prompt/session state. | Verify HaloKV namespaces never alias RPC tensor identities; injected RPC loss must fail or recompute without accepting partial prompt state. | No rank-global checkpoint or two-phase distributed restore protocol exists in this source map. Rank ownership and fallback remain OPEN. |

## Required compatibility fixtures before implementation qualification

1. **FX-STATE-TRANSFORMER:** sequence save/restore, same slot and different destination sequence, with next-token equivalence.
2. **FX-STATE-RECURRENT:** recurrent boundary round-trip and incompatible-position rejection.
3. **FX-STATE-HYBRID:** full, partial-only, ISWA, DSA/DSV4, and attention/recurrent boundary consistency.
4. **FX-STATE-TARGET-DRAFT:** target/draft atomicity; one missing or corrupt half rejects the entry.
5. **FX-STATE-MTP:** pending current/previous hidden state, positions, dimensions, and type checks.
6. **FX-STATE-EAGLE:** typed state round-trip and wrong-implementation rejection.
7. **FX-SAMPLER-REPLAY:** sampler/RNG/grammar/reasoning replay equivalence over multiple subsequent tokens; if it fails, persistence stops at prompt-safe boundaries until a codec exists.
8. **FX-SLOT-LIFECYCLE:** quiesce, cancellation, reuse, cross-slot restore, and clean rollback after partial load.
9. **FX-OUTPUT-BOUNDARY:** disconnect/retry/cancel at each partial/final boundary with declared at-most-once or retry semantics.
10. **FX-CACHE-TRANSACTION:** crash injection at temp write, file sync, rename, directory sync, catalog publish, eviction, and deletion; invalid state always misses/recomputes.
11. **FX-COMPAT-IDENTITY:** independently mutate model hash, tokenizer/template, architecture, state format, build/ABI, speculative mode, cache policy, topology/rank, and tenant identity; every incompatible mutation must miss.
12. **FX-RPC-SEPARATION:** transport loss and rank failure cannot cause RPC tensor-cache identity to authorize prompt-state reuse; single-node fallback behavior must be explicit.

## OPEN design gates

- **OPEN-IDENTITY:** exact canonical compatibility tuple and cryptographic digest construction.
- **OPEN-SAMPLER:** replay-only safe boundary versus a new complete sampler/grammar/RNG codec.
- **OPEN-SPEC-MULTI:** behavior when more than one speculative implementation state is active.
- **OPEN-ROLLBACK:** proof that a failed target/draft/spec load leaves no partially accepted destination state.
- **OPEN-LEASE:** slot quiescence, provider leases, concurrent readers/writers, and cancellation ownership.
- **OPEN-OUTPUT:** output commit and retry semantics remain outside prompt-cache persistence.
- **OPEN-TENANT:** authenticated namespace derivation, quotas, deletion authorization, and audit record.
- **OPEN-DISTRIBUTED:** rank ownership, compatible topology identity, coordinated publish/restore, failure handling, and single-node fallback.
- **OPEN-MIGRATION:** unknown/old state versions, catalog upgrades, quarantine, and recoverable deletion.

## Static verification record

The following candidate blobs were checked with `git cat-file -e` and matched the planning baseline:

- `include/llama.h`: `4b6a4c563046260ec437466ebf63cd49ed00822d`; `src/llama-context.cpp`: `01fd4c697a14d899b355bd38de94e5b63bd0fc92`; `src/llama-context.h`: `87fdb0294abbe06d47a1d2dbb02792aa143f3b02`.
- `common/common.cpp`: `5b92940284b52d90e4cae9d8c5af0b4ca7080448`; `common/common.h`: `912639e93752310ab8ad2c5206de2f1d62301f30`.
- `common/speculative.cpp`: `79bebab67548222e344b8f63cd5f6ca6135705c1`; `common/speculative.h`: `6ce82e5afd672a0e1b36e2ef04133da428467c54`.
- `common/sampling.cpp`: `d16300664ddd37721ae52fcf1f8c27d14a0dfb73`; `common/sampling.h`: `91622debd65fb12551649c4fa75ffe815b07edf0`.
- `tools/server/server-context.cpp`: `f7d0bda8dfbdb1425a203ffa497b8f6f8061144f`; `tools/server/server-task.cpp`: `6e0bb85d1080ac57746eeef0aacddfa293d90fd8`; `tools/server/server-task.h`: `a3ebc1ef5a4a9c88d4045a32a30e857ba6af7087`.
- `src/llama-kv-cache.cpp`: `7afd9e5c86f69817e242e7a1463e4ba52e7b5af7`; `src/llama-memory-recurrent.cpp`: `addc427b620de54a8e8deaf17542c3d7b1be943f`; `src/llama-memory-hybrid.cpp`: `529022ded18d8cf9e354af79e2535213671f68a1`.
- `ggml/src/ggml-rpc/ggml-rpc.cpp`: `1cb8f563d8583db966c424b7f2fd58f65f8f9c7f`; `ggml/include/ggml-rpc.h`: `ad9232b89c4ab10cfa7d7afbf374c212b898584e`.

No node was contacted and no code was built or executed. Runtime qualification, performance, compatibility, and distributed correctness remain unclaimed.
