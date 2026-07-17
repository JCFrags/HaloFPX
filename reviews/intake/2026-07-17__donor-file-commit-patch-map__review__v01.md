# Donor file/commit patch map: CachyLLama and llama-ai into HaloFPX

- Review date: 2026-07-17
- Status: **reviewed planning evidence; no implementation approval**
- Scope: read-only source/history analysis of preserved local reference clones and imported research packages
- Target: a future user-controlled HaloFPX fork based on `charlie12345/ROCmFPX`
- Disposition vocabulary: `SAFE TRANSPLANT` means a narrow MIT source adaptation may be considered after tests; `CLEAN REIMPLEMENTATION` means preserve behavior/specification but do not copy the implementation; `TARGET-NATIVE` means use ROCmFPX's existing implementation; `REJECT` means do not import.

## 1. Executive result

Do **not** merge or cherry-pick the CachyLLama head, its initial SSD commit, or the llama-ai wrapper into HaloFPX.

The useful donor behavior is not a self-contained patch:

- CachyLLama `6be745998f568e379ea197fcf827baec73ff9940` is 53 fork commits ahead of comparison parent `92366df30d4eaa4b85139b5fd694360237731b19` and is also a merge of that parent.
- The persistent state path begins with `6a0db500ca1058e06a232c02c50eb5df56b0d151`, a broad commit that mixes storage, server scheduling, llama state internals, hybrid memory, Metal kernels, a disconnected page-manager prototype, and backend tests.
- Correct behavior at the donor head depends on at least 26 later cache-related fixes and integrations. Several fix restore correctness across restart, hybrid/recurrent models, MTP, destination sequence IDs, and slot eviction. Selecting only the headline commit would knowingly reintroduce fixed faults.
- ROCmFPX already has a stronger default-off prompt-cache transaction core in `tools/server/server-task.{h,cpp}`: private owned run directories, locking, bounded LRU, temporary writes, validation, atomic rename, directory sync, corruption fallback, a disk-write circuit breaker, and focused Python tests.
- The missing capability is restart-persistent, compatibility-keyed, authorization-aware cataloging and lifecycle. That should be added as a new provider/catalog layer around the ROCmFPX cache/state seam, not by replacing the target core with CachyLLama's v3 files and slot/conversation directory model.
- `fewtarius/llama-ai` is GPLv3. Its runner/profile/deployment behavior is useful as an operational specification only. Keep it in a separate GPL deployment project or independently specify and implement equivalent configuration; do not copy its shell/service files into an MIT HaloFPX engine tree without an explicit license decision.

Recommended implementation posture:

1. Freeze the selected ROCmFPX candidate (currently compare both `a5605a72768c6562241b248e268e33dc92787394` and captured `61f2f2d7bc4955e9bca821095ef69125837133b5`). The latter changes only backend/quantization paths relative to the frozen pin; all inspected server/cache blob IDs are identical.
2. Keep `server_prompt_cache` as the initial storage/state provider and make persistence a default-off lifecycle policy.
3. Add a clean, versioned persistent catalog/manifest with cryptographic identity, exact compatibility, immutable publication, tenant authorization, quotas, inspection, and typed miss reasons.
4. Add narrow state adapters only where ROCmFPX lacks proven semantics for hybrid/recurrent and draft/MTP components.
5. Treat CachyLLama commits as provenance and regression-test seeds, not as an ordered cherry-pick series.

## 2. Exact source identities and comparison boundaries

| Role | Repository and commit | Comparison boundary | Root license observed | Local reference |
|---|---|---|---|---|
| Cache/server donor | `fewtarius/CachyLLama@6be745998f568e379ea197fcf827baec73ff9940` | Merge parent and merge base `ggml-org/llama.cpp@92366df30d4eaa4b85139b5fd694360237731b19`; donor is 53 commits ahead at the preserved graph | MIT | `sources/repositories/fewtarius__cachyllama/` |
| Operational wrapper | `fewtarius/llama-ai@1017f3dfdce3ca2b06aa9007b23295db3bb35722` | Parent `d8a07baad6ab175f8badbc4d496c9190b0cc3b2d`; head changes the `CachyLLama` gitlink to the exact donor head | GPLv3 text; files carry GPL-3.0-or-later notices | `sources/repositories/fewtarius__llama-ai/` |
| Frozen target | `charlie12345/ROCmFPX@a5605a72768c6562241b248e268e33dc92787394` | Project Wiki frozen candidate | MIT plus third-party notices | `sources/repositories/charlie12345__rocmfpx/` |
| Captured target head | `charlie12345/ROCmFPX@61f2f2d7bc4955e9bca821095ef69125837133b5` | One commit after frozen target: `61f2f2d7...` | MIT plus third-party notices | same clone |
| Canonical upstream reference | `ggml-org/llama.cpp@788e07dc91d266ad3162a1ce9037665656269689` | Wiki baseline | MIT | `sources/repositories/ggml-org__llama.cpp/` |

The preserved clones are evidence only. No checkout, fetch, pull, build, test, or file edit was performed in them.

## 3. Target overlap that controls the port

At both ROCmFPX target candidates, these relevant blobs are identical:

| Path | ROCmFPX blob at both `a5605a7` and `61f2f2d` | CachyLLama head blob | Consequence |
|---|---|---|---|
| `tools/server/server-task.cpp` | `6e0bb85d1080ac57746eeef0aacddfa293d90fd8` | `3b70e51937cd044e82a7aa7b42ecbb4c1c5b4bc2` | Preserve target transaction/cache implementation; do not overwrite. |
| `tools/server/server-context.cpp` | `f7d0bda8dfbdb1425a203ffa497b8f6f8061144f` | `14ca8eda44287a383aef3a24a1878aabbcb4c398` | Main conflict surface; integrate through a narrow provider seam. |
| `tools/server/server-models.cpp` | `698489a11e2f08c1d335a4a8a2a9399e002534f6` | `d1fdc06079c886cdd996d0f5681f2b25905ac73e` | Router/lifecycle already exists target-native. |
| `tools/server/server.cpp` | `af93bd8856ee8a3ee002351e06f6e5f601079332` | `eea9398464b8d24e1022ad529e14c8a26a817a53` | Use target routes and authentication policy. |
| `common/arg.cpp` | `209091f42a9bc3d4ad7a5b7d0274f47b85abff84` | `5d48ca6f3030b9f0dee807c9bea45ff80aa9dfc3` | Extend target configuration; do not import donor defaults wholesale. |
| `common/common.h` | `912639e93752310ab8ad2c5206de2f1d62301f30` | `76bd53dbd288ee286e2f0acf3a9d7d27a93b21b9` | Add new fields behind explicit persistent mode. |
| `include/llama.h` | `4b6a4c563046260ec437466ebf63cd49ed00822d` | `d55eca769ef64067b93402d6ee03bca7ba8f75ad` | APIs are not ABI-equivalent; use an adapter and symbol audit. |
| `src/llama-context.cpp` | `01fd4c697a14d899b355bd38de94e5b63bd0fc92` | `f21ad85662a3f09a73d4f8a38644ffab4d00a28f` | No source transplant until state-component equivalence is proven. |
| `src/llama-kv-cache.cpp` | `7afd9e5c86f69817e242e7a1463e4ba52e7b5af7` | `a6825ba77c9f077292e01182d34edd44f7162d89` | IMROPE and cache-shift logic require target-specific audit. |
| `src/llama-memory-hybrid.cpp` | `529022ded18d8cf9e354af79e2535213671f68a1` | `668f8f258c234cf21d0909f8ed4409e7c10f9d6d` | Hybrid restore is high coupling/high risk. |
| `common/speculative.cpp` | `79bebab67548222e344b8f63cd5f6ca6135705c1` | `47e568d164886ff2c1d5dfb6b45b91cf27f5b47e` | Draft/MTP state must use typed target adapters. |

CachyLLama-only storage/orchestration files at the donor head are:

- `common/kv-ssd-cache.cpp` blob `9fe3d2bfb646cb00c1f50c06fde4321d3da8632d` (1,214 lines)
- `common/kv-ssd-cache.h` blob `4c13dc525366d57b8da5c361fbc36f32480ee64a` (236 lines)
- `common/kv-ssd-system-cache.cpp` blob `4c4adfb9a90fda87d8dcd6b803c05f7d6f934488` (701 lines)
- `common/kv-ssd-system-cache.h` blob `e4a360449c6c8f935d1d3c6d692023b56f5e730d` (198 lines)
- `common/kv-ssd-posix.h` blob `7bae9e1016b33abb17e0159eb20edcc2249bdb1c` (148 lines)
- `tools/server/server-context-page-manager.cpp` blob `91099bd380791cfb66aef6f3ef37ea70c192ae24` (695 lines)
- `tools/server/server-context-page-manager.h` blob `fe2e6d61beeb16b397a4fb2606ed436c7ee56a01` (217 lines)
- `tools/server/server-context-ssd-cache.cpp` blob `30512ce248a3c0a1656f6df42edbdea46b829eb4` (127 lines)
- `tools/server/server-context-ssd-cache.h` blob `b4c60b4c23991d27719dd9ecce61415d4db51de0` (96 lines)

Their presence does not make them safe transplant units: they depend on modified target files and carry the weak identity/publication semantics rejected by Sections 56–65.

## 4. Behavior-level donor patch map

### 4.1 Persistent sequence/KV checkpoint state

**Desired behavior:** restart-persistent target state, hot/warm/cold tiers, bounded cold objects, state-component support, deterministic miss/fallback, and measurable saved prefill.

| Donor commit or range | Principal paths | What it contributes | Coupling and dependencies | Disposition |
|---|---|---|---|---|
| `6a0db500ca1058e06a232c02c50eb5df56b0d151` | `common/kv-ssd-cache.{h,cpp}`, `tools/server/server-context-{page-manager,ssd-cache}.{h,cpp}`, `server-context.cpp`, `server-task.{h,cpp}`, `include/llama.h`, `src/llama-{context,kv-cache,memory-hybrid}*`, `common/CMakeLists.txt`, plus Metal/backend/prototype files | Initial SSD-backed checkpoint and hybrid restore | **Very high.** Mixed-purpose root commit; storage, server slots, llama state ABI, hybrid memory and unrelated backend work are entangled | `REJECT` cherry-pick; use as provenance/specification only |
| `3e38e4a7`, `afa4323d`, `f1921da7`, `cedeaccc`, `83af232a` | cache + page manager + server context | Cold-start LCP validation, continuation fixes, same-conversation rules, overlap output, max-token guard | Depends on initial storage and slot/conversation model | `CLEAN REIMPLEMENTATION` of tested semantics |
| `96937d47`, `bce3f53f` | `common/kv-ssd-cache.cpp`, system cache, `server-context.cpp` | Defers checkpoint until first token; fixes dense short-prompt checkpoint omission | Scheduler/event-order dependent | `CLEAN REIMPLEMENTATION`; derive explicit lifecycle state machine |
| `9b06c5a9`, `835013b3`, `ef67a59c`, `de62cdfb` | `server-context.cpp`, `src/llama-context.cpp`, `common/common.h` | Hybrid/recurrent warm restart and attention-only guard fixes | High model-state coupling | `SAFE TRANSPLANT` only as line-level semantic references into a target adapter; no direct file copy |
| `85a0c11e` | cache records, speculative state, page/SSD adapters, server context | Persists/restores `ctx_dft` and `pending_h` for MTP | High: target/draft state versions and speculative API | `CLEAN REIMPLEMENTATION` as typed optional/required components; validate target-only fallback |
| `a461a1fc`, `c8ead677` | page manager, SSD adapter, server context | Restores KV cells under destination slot sequence ID; completes continuation path fix | Slot/seq ownership critical | `CLEAN REIMPLEMENTATION`; mandatory cross-slot regression tests |
| `3162f615` | `tools/server/server-context.cpp` | Sidecar context checkpoint for slot save/restore | Coupled to slot API and donor file format | `REJECT` as native format; consider an offline legacy importer only |
| `a4e459ef` | cache, args/config, page manager, server context | `--cache-ssd-no-fsync` | Durability-policy coupling; unsafe naming/default risk | `CLEAN REIMPLEMENTATION` as explicit durability enum; never call unsynced mode “durable” |
| `8b2cf6c6`, `9d28796f` | cache/system cache/POSIX helper | Large-context errors, thread safety, portability, logging | Useful defect catalogue, but attached to rejected storage | `CLEAN REIMPLEMENTATION` tests and error semantics |
| `024dff23` | `server-context.cpp` | Evicts highest `pos_min` checkpoint instead of insertion-oldest | Donor slot-local heuristic | `REJECT` policy; target should use byte/quota/reachability-aware eviction |

The above is a minimum corrective closure, not a cherry-pick roster. The behavior at `6be7459` also spans upstream merge commits `6fa9d40d`, `abe63a5a`, and `6be7459`; transplanting those merges would import unrelated upstream history and is prohibited.

**Target-native overlap:** ROCmFPX already supplies RAM/disk prompt-state storage and LCP matching in `tools/server/server-task.cpp`, configured through `common/{common.h,arg.cpp}` and integrated by `tools/server/server-context.cpp`. Its disk cache is deliberately per-run and removed at shutdown. Extend that ownership and transaction model with a persistent catalog; do not replace it.

**Required new dependencies:** cryptographic digest implementation already acceptable to the project; canonical compatibility encoding; filesystem abstraction for no-replace/atomic publication and sync semantics; versioned component codecs; quota/GC registry; authenticated principal-to-opaque-scope adapter; clock-independent generation IDs; bounded recovery scanner.

**Required tests before acceptance:** restart suffix-only equivalence for dense transformer, hybrid/recurrent, MTP/draft and speculative modes; destination unchanged on failure; one-field compatibility invalidation; truncated/swapped/corrupt/oversized component; ENOSPC/EIO/kill at every write boundary; concurrent readers/writers; exact disk quota; old-or-new recovery; target-only fallback when optional draft state is unusable; rank-local ownership and all-rank failure behavior for distributed mode.

**Risk:** critical. A false hit can silently corrupt generation. Any uncertainty is a typed miss/recompute.

### 4.2 Prefix catalog, system-prefix reuse, and continuation matching

| Donor commit or range | Principal paths | Behavior | Coupling | Disposition |
|---|---|---|---|---|
| `56dca0825e1d0b3a4b5f00a1fc1e59f2c6a900f8` | `common/kv-ssd-system-cache.{h,cpp}`, `common/CMakeLists.txt` | Global system-prompt state pool | Medium storage, high trust-boundary risk | `REJECT` storage format/hash; `CLEAN REIMPLEMENTATION` behavior |
| `cc60f891`, `70ceb7b1`, `926f343f`, `dffcd625` | `common/{arg.cpp,common.h}`, `server-context.cpp` | Lifecycle wiring, hybrid enablement, compatibility alignment, rebase restoration | High server lifecycle coupling | `CLEAN REIMPLEMENTATION` behind caller-supplied explicit rendered-token boundary |
| `521db277` | `kv-ssd-system-cache.cpp`, `server-context.cpp` | Heuristic GLM/Gemma system boundary detection | Tokenizer/template heuristic | `REJECT`; caller/template layer must supply exact boundary |
| `43d781ac` | `kv-ssd-cache.{h,cpp}`, page manager, SSD header | POSIX readahead for selected cold checkpoint | OS-specific, synchronous read path | `SAFE TRANSPLANT` concept after usefulness/pressure metrics; use target portability layer |
| `3e38e4a7` through `c8ead677` corrective chain | cache/page manager/server context | LCP selection, thresholding, overlap, continuation, destination seq ID | Conversation/slot identity and donor directory scans | `CLEAN REIMPLEMENTATION` catalog lookup and candidate scoring |

**Keep:** exact token-prefix comparison, absolute minimum plus ratio threshold, longest-prefix preference, optional prefetch hints, and scope-aware reuse.

**Redesign:** persistent index rather than global directory scans; collision-resistant token/input digest; full exact token verification before restore; explicit `PREFIX_INFERENCE_STATE` versus `SESSION_CHECKPOINT` kinds; authenticated/authorized scope before lookup; system sharing only for administrator-approved immutable rendered prefixes; immutable catalog generations and bounded recovery.

**Reject:** FNV identifiers as trust gates, bounded-prefix-only verification, fuzzy anonymous cross-directory restore, slot IDs or conversation directory names as durable identity, heuristic role-marker boundary detection, and cross-tenant similarity as authority.

**Target overlap:** ROCmFPX already performs process-local/disk LCP matching and has `--slot-prompt-similarity`. The new catalog should feed verified candidates into that selection policy rather than add a second competing slot chooser.

**Tests:** exact-match and longest-prefix table tests; short-prefix rejection; template/tokenizer/adaptor/image/position-mask invalidation; authorized tenant/public/anonymous matrices; hash collision fixtures; catalog rebuild; stale index; useful-prefetch ratio and page-cache pressure; no existence/timing disclosure before authorization.

**Risk:** critical for privacy and correctness.

### 4.3 Request lifecycle, checkpoint lifecycle, and model lifecycle

Three different lifecycles must remain separate:

1. request/slot state;
2. persistent checkpoint/catalog state;
3. model process/router state.

**Checkpoint lifecycle donor provenance:** `96937d47` and `bce3f53f` expose ordering defects; `3162f615` couples manual slot save/restore to a sidecar; `a4e459ef` exposes durability policy; `8b2cf6c6` and `9d28796f` expose failure/thread-safety needs. Use these as state-machine test cases, not code donors.

**Model lifecycle:** `tools/server/server-models.cpp` and `tools/server/server.cpp` already exist in CachyLLama comparison parent `92366df...` and in ROCmFPX `a5605a7`; the CachyLLama fork delta contains no standalone model-router capability commit after that base. Load, unload, autoload, reload, child-server proxying, model discovery, LRU unload, and SSE are therefore inherited/upstream-overlap behavior, not CachyLLama donor patches.

Disposition: `TARGET-NATIVE`. Do not transplant CachyLLama router files. Add only HaloFPX policy around target-native routes: authentication/authorization, allowlisted presets/paths, memory admission, drain/readiness, pinned models, audit log, and safe fallback. Router mode remains experimental and must not be exposed directly to untrusted clients.

The llama-ai wrapper adds Hugging Face discovery/download and profile selection in `llama-run.sh` (GPL blob `3684a299361b3ca45ff57656a43073f8b9629047`). This is `CLEAN REIMPLEMENTATION` outside the engine with checksums, free-space checks, allowlisted destinations, split-file completeness, atomic downloads, and an explicit lifecycle controller.

**Tests:** checkpoint state-machine transition table; cancel during save/restore; checkpoint after first evaluated token; drain before unload; load failure/timeout; autoload thrash; reload atomicity; child crash; cache generation fenced across model reload; single-node safe mode.

### 4.4 Scheduling, slot affinity, quotas, and isolation

| Donor commits | Paths | Behavior | Disposition and reason |
|---|---|---|---|
| `ae7101da` | `common/kv-ssd-cache.{h,cpp}`, `docs/development/user-isolation-design.md` | Namespace prefix | `CLEAN REIMPLEMENTATION`; opaque authenticated scope, not raw user-derived path |
| `7043bf19` | `server-chat.cpp`, `server-context.cpp`, `server-task.{h,cpp}` | Threads body `user_id` into tasks | `REJECT` as authorization; request field is only an untrusted hint |
| `cfbdfb39` | `common/common.h`, `server-context.cpp` | Per-user concurrency and affinity | `REJECT` implementation due lifecycle/accounting defects; retain desired policy |
| `6b7207d0` | page manager + server context | Routes user to namespace; blocks cross-user scan | `CLEAN REIMPLEMENTATION` after authenticated scope derivation |
| `7bdf8c25` | server common/context/queue | HTTP 429 for cap | `SAFE TRANSPLANT` response semantics into target admission controller |
| `8b9fa2e6` | `common/arg.cpp`, server README/design doc | CLI cap | `CLEAN REIMPLEMENTATION` in one versioned config schema |
| `602bf12d` | `server-context.cpp` | Restores isolation/hybrid safety lost in rebase | Evidence of fragile coupling; no cherry-pick |

Known donor defects at the exact pin make the implementation unsuitable as a security boundary: a moved-from task vector is read on one HTTP fast path; `release()` clears `user_id_` before decrementing the per-user count; and prompt-similarity selection can expose residual slot state across users. The body-provided ID is not authenticated, file permissions are not a complete tenant boundary, and raw IDs can enter logs.

**Target design:** one admission/selection policy owns principal scope, per-scope concurrency, fair queueing, affinity preference, reusable-prefix score, quota, cancellation, slot release, and residual-state reset. `prefer` affinity may not starve other scopes. Authorization precedes cache lookup. Anonymous persistent sharing defaults off.

**Tests:** authenticated A/B isolation; forged body IDs; lifecycle accounting on every release/error/cancel path; slot reuse residual-state test; fairness/starvation; 429 schema and retry behavior; cross-scope cache timing; quota/eviction agreement; concurrent same-principal and anonymous traffic.

**Risk:** critical security/correctness.

### 4.5 HTTP/API and observability

Most general HTTP, Responses, Anthropic, metrics, slots, router, schema, embeddings, rerank, TLS and Web UI behavior is inherited from llama.cpp and already overlaps ROCmFPX. It is not a CachyLLama patch lane. Use target-native interfaces and verify conformance at the chosen target commit.

Narrow CachyLLama delta:

| Commits | Paths | Capability | Disposition |
|---|---|---|---|
| `1ffbac42` | `include/llama.h`, `src/llama-context.{h,cpp}` | MoE expert activation tracking API | `DEFER`; useful for placement research but modifies public/internal API and needs overhead/accuracy proof |
| `b69de849` | above plus `src/llama-model.cpp`, `server-context.{h,cpp}`, `server.cpp` | Server expert statistics | `CLEAN REIMPLEMENTATION` only if a named experiment requires it; low-cardinality bounded telemetry |
| `2f1fc4b1` | `server-context.cpp` | Reset/LCP probe logging | `REJECT` as permanent ad hoc logging; convert to structured debug trace with redaction/rate limits |
| Cache stats embedded across `kv-ssd-cache.h`, page manager, server context | several | Hits, misses, bytes, tiers, restore timing | `CLEAN REIMPLEMENTATION` in one target cache registry; donor omits complete user-cache aggregation |

Minimum persistent-cache observability:

- lookup result by bounded reason (`hit_ram`, `hit_run_disk`, `hit_persistent`, `miss_absent`, `miss_incompatible`, `miss_corrupt`, `miss_unauthorized`, `miss_incomplete_rank`);
- bytes read/written, logical versus device writes when measurable, restore/validation latency, tokens/prefill saved, prefetch useful/wasted, evictions/quota rejects, recovery/quarantine, circuit state;
- no raw prompts, user IDs, cache keys, paths, token arrays, or high-cardinality checkpoint labels;
- optional response field that reports cache source without exposing another tenant's object existence.

**Tests:** target API conformance, metrics cardinality limits, counter conservation, user-cache aggregation, redaction, disabled-by-default expert telemetry, telemetry overhead, cancellation and error response schemas.

### 4.6 Configuration, build, and deployment from llama-ai

All entries below are GPL-wrapper evidence. None is an MIT engine transplant.

| Behavior | Exact wrapper provenance | Current blob/path | Disposition |
|---|---|---|---|
| Base AMD APU runner, model download, backend selection | `750f43299625ff0de8e7d84652f600f31b275bf7`; expanded in `693914f3425965d4762de5cb80be1b63eb0033d1` | `llama-run.sh` blob `3684a299...` | `CLEAN REIMPLEMENTATION` or separate GPL deployment repo |
| Strix Halo hardware-tier profiles | `34b16c2793f7b6bdb6ef8a0baf7dbe37dccd7d51` | `llama-run.sh`, `scripts/detect-gpu.sh` blob `4f2a4bbedc82a610f1852b81759016347562e1ca` | Use as hypotheses only; replace static guesses with measured versioned profiles |
| System-prefix options | `035034059c7229fba8a1bf2d7ad11fa01ad71654`, refactor `6b505d36ef00f07190682385309cfb4644a406d6` | `llama-run.sh` | `CLEAN REIMPLEMENTATION` through generated config |
| SSD RAM budgets | `5647ee7535858a39973cfc50c70de10416ef7947` | `llama-run.sh` | Retain explicit budgets; no copied shell |
| Halo ring sizing and MoE cache disable | `a1841fa4`, `933a98d0`, `6cb480ad` | `llama-run.sh` | `DEFER`; benchmark on target workloads; persistence and RAM policy must be independent |
| GGUF architecture detection, SSD sentinel, no-fsync | `0411eda7`, `de3bfc41` | `llama-run.sh` | `CLEAN REIMPLEMENTATION`; do not trust filename/profile heuristics alone |
| Dense/hybrid classification and SSD application | `02a08da6`, `18271a88` | `llama-run.sh` | Regression-test seeds; use engine metadata, not shell heuristics |
| SSD opt-out | `cad73d5c909a08dd9097eb40516fe9490a4e1b92`; merged by `d8a07baa` | `llama-run.sh` | Retain explicit `off/run/persistent` lifecycle selector |
| ROCm environment/build repair | `fb13edf5a6141902eb65c1d7b51cadc13f5c0fa2` | `scripts/env.sh` blob `4a6e94d...`, `src/cachy-llama-rocm/build.sh` blob `170fbf8e...`, rebuild blob `dedf0d29...` | Re-specify for pinned ROCm/CMake packages; do not vendor host paths |
| TTM/GTT tuning helper | `34b16c27`, fixes `1a41d231`, formatting `560285ac` | `scripts/apply-ttm-kernel-params.sh` blob `76b50c10...` | Keep outside engine; privileged, reversible, measured profile only |
| systemd unit | introduced/current through `0bf6ae36c440618d80d32f5388a0edbc107ac919` | `systemd/llama-server.service` blob `f991dae...` | `REJECT` direct use: host-specific `/home/deck`, stale flag, drifted profile; generate from versioned config |
| dependency installer | commits including `34b16c27`, `d84c2af8`, `54e9e0e6`, `78c77694`, `d5c5189e`, `560285ac` | `scripts/install-deps.sh` blob `77ee08d...` | `REJECT` execution/import; create distro- and version-pinned prerequisite document |

The pinned service passes `--checkpoint-every-n-tokens`, while the wrapper accepts `--checkpoint-min-step`; this is wrapper/service drift. It is direct evidence that deployment artifacts must be generated and validated from one schema.

**Recommended config precedence:** command-line override > one versioned deployment config > named measured hardware/model profile > conservative code default. Emit the resolved config and provenance at startup. Cache lifecycle, durability, quota, prefix sharing, and RAM tier policy are independent fields.

**Tests:** JSON/schema validation, unknown/stale option rejection, profile golden snapshots, cgroup/available-memory admission, systemd dry-run/argument parity, rollback unit, owner-only cache permissions, secrets excluded from argv/logs, ROCm/gfx1151 smoke test, backend flags and binary hash receipt.

## 5. Explicit reject list

Do not import or normalize any of the following into HaloFPX:

1. CachyLLama merge head or upstream merge commits.
2. Initial mixed SSD commit `6a0db500...` as a cherry-pick.
3. `common/kv_page_manager.{h,cpp}` and root `test_kv_page_manager.cpp`; the duplicate prototype is not linked into the active common library and has inconsistent scale comments/literals.
4. CachyLLama v3 checkpoint/system-cache formats as native HaloKV storage.
5. FNV-based object, user, conversation, prefix, or compatibility identity as a trust decision.
6. Direct-final-path publication, optional sync presented as durability, or acceptance without full content verification.
7. Global anonymous continuation directory scans or fuzzy cross-conversation restore.
8. System-boundary detection by decoded role-marker heuristics.
9. Raw request `llama_user_id` as authentication, authorization, namespace ownership, or logging identity.
10. Donor slot-affinity/concurrency implementation in its current lifecycle ordering.
11. Donor checkpoint-count/mtime/highest-position eviction as the persistent quota policy.
12. Direct copies of GPL llama-ai shell scripts, service unit, profiles, installer, or documentation code into an MIT engine repository without an explicit license decision.
13. CachyLLama's copied router/API files where ROCmFPX already has target-native equivalents.
14. Any cache hit that is partial, mixed-generation, cross-rank incomplete, or imported into a live destination before all validation succeeds.

## 6. Proposed implementation lanes and dependency closure

This is a planning dependency graph, not authorization to implement:

| Lane | Depends on | Candidate source references | Acceptance boundary |
|---|---|---|---|
| L0 Baseline and conformance | selected ROCmFPX commit, build/toolchain/model pins | target `a5605a7` vs `61f2f2d`; cross-fork test intake | Clean build and existing target cache tests on both nodes; no behavior change |
| L1 Storage-provider seam | L0 | target `server_prompt_cache`; donor adapter boundaries | Default behavior and ABI unchanged; current tests green |
| L2 Object kinds, compatibility and codecs | L1 | donor typed target/draft/spec concepts; Sections 57/61 | Canonical vectors; every mismatch typed miss; destination unchanged |
| L3 Persistent object publication/catalog | L2 | target transaction core; donor restart/catalog behavior only | Atomic old-or-new recovery, cryptographic verification, bounded startup |
| L4 Prefix/system catalog | L3 | donor LCP and prefetch concepts | Authorized exact lookup, explicit rendered boundaries, no fuzzy global scan |
| L5 Lifecycle, quota and administration | L3 | donor retention/observability defects as tests | Byte/tenant quotas, dry-run inspect/prune, active-reader-safe GC, audit |
| L6 Scheduling/isolation | L4, authenticated principal contract | donor `ae7101..8b9fa2` behavior specification | Fair admission, exact accounting, residual-state isolation, no hint-as-auth |
| L7 Hybrid/recurrent/MTP adapters | L2 plus model-specific state contract | donor `9b06c5`, `835013`, `85a0c1`, `a461a1`, `c8ead6` | Suffix-only equivalence per architecture and safe component fallback |
| L8 Distributed rank-local commit | L3, execution-plan/topology authority | HaloKV Sections 58/63; no direct Cachy donor | All-rank global commit; mixed/incomplete generations rejected; single-node fallback explicit |
| L9 Deployment profiles | L0 and measured experiments | llama-ai GPL behavior specification | Generated config/service, no stale flags, reproducible rollback |

No lane may silently modify target defaults. Each accepted lane needs donor/source SHA, copied/adapted paths, authorship and notices, dependency closure, target commit, tests, validation receipt, and rollback.

## 7. Test inventory and gaps

CachyLLama's entire 53-commit delta adds only `test_kv_page_manager.cpp` and modifies `tests/test-backend-ops.cpp`; no focused restart-persistence, format corruption, atomic publication, template-boundary, tenant-isolation, concurrent-instance, or real active-server cache suite was found in the delta. The standalone page-manager test does not test the active server path.

ROCmFPX has the better seed suite at `tools/server/tests/unit/test_prompt_cache_disk.py` plus test harness controls in `tools/server/tests/utils.py`. Extend those tests first. Do not use donor README performance claims as an oracle.

Minimum promotion suite:

- unchanged default-off behavior and API/ABI smoke tests;
- deterministic suffix-only restore against full recomputation;
- dense, hybrid/recurrent, MTP/draft/speculative component matrix;
- exact compatibility mutation matrix;
- corruption, truncation, oversize, swap, replay and unknown-schema cases;
- crash/kill/ENOSPC/EIO matrix across data, sync, publish, catalog and directory-sync points;
- concurrent process/read/write/GC tests;
- authenticated tenant and anonymous/public sharing tests;
- lifecycle/cancel/release/accounting/fairness tests;
- measured cold/warm/hot latency, tokens saved, memory/page-cache duplication, SSD logical/physical writes and wear metadata;
- rank-local restore, missing/corrupt/delayed rank, topology epoch mismatch, whole-restore miss, and explicit single-node fallback.

## 8. Licensing and provenance gate

The preserved manifests record MIT root licenses for CachyLLama, ROCmFPX and llama.cpp, and GPLv3 for llama-ai. This review is not legal advice.

For MIT-derived adaptations, retain original authorship, source commit, path/blob identity, license text/notice obligations, and a mapping from donor lines/semantics to the adapted commit. The broad initial commit and later fixes have distinct authorship that must not be collapsed into “from CachyLLama.”

For llama-ai behavior, the project should choose one of two explicit boundaries:

- a separate GPL deployment repository that invokes the MIT HaloFPX binary; or
- a source-independent behavioral specification and separately authored implementation, with recorded reviewer/implementer exposure and independent tests.

Calling a rewrite “clean-room” is not itself proof of legal separation. A maintainer/license decision is required before copying wrapper code, comments, profile tables, or service content.

## 9. Blockers and remaining uncertainty

This patch map is sufficient to avoid blind merging, but implementation remains blocked on these decisions/evidence:

1. Maintainer-approved repository authority and the exact first HaloFPX base (`a5605a7` versus `61f2f2d` or a later explicitly audited commit).
2. Exact object-kind and required-state-stream contract for prefix state versus exact session continuation.
3. The ROCmFPX state API symbol/component map for transformer, hybrid/recurrent and MTP at the chosen base, including source versus ABI compatibility.
4. Authenticated principal and anonymous/public-sharing policy.
5. Persistent catalog encoding, compatibility fingerprint, quota/GC, encryption/key policy, and platform filesystem support.
6. Distributed rank ownership, topology identity, global commit authority, and complete single-node fallback predicate.
7. License/maintainer approval for MIT adaptations and the GPL wrapper boundary.
8. Machine results on both Strix Halo nodes. Static source analysis cannot approve performance or runtime correctness.

## 10. Evidence used

Primary local sources:

- `sources/repositories/manifest.yaml` and `2026-07-17-clone-receipt.md`
- read-only Git objects in the four reference clones
- `wiki/HaloFPX_Wiki/03_Repository_and_Engineering/{11,13,14,15}_*/`
- `wiki/HaloFPX_Wiki/09_HaloKV_Persistent_Cache/56_CachyLLama_Cache_Semantics_and_Porting_Map/`
- imported `llama-ai-cachyllama-feature-inventory` and `ROCmFPX-CachyLLama-Integration-Wiki`
- intake reviews `2026-07-17__integration-feature-inventory__review__v01.md` and `2026-07-17__halokv-cache__review__v01.md`
- `references/agent-harness.md` and the canonical Agent Harness evidence-promotion/review guidance

## 11. Review decision

**ACCEPT this file as a donor provenance and implementation-planning map.**

**DEFER all code import.** The preferred route is a target-native persistent-cache provider/catalog extension with narrow, individually tested state adapters. The current direct cherry-pick roster is intentionally empty.
