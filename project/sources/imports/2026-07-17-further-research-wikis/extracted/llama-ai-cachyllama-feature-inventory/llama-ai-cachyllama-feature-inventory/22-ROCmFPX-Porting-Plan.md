# ROCmFPX porting plan

Target baseline: `charlie12345/ROCmFPX@a5605a72768c6562241b248e268e33dc92787394`.

## Governing decision

Do not replace ROCmFPX's current `server_prompt_cache` disk path. It already contains the strongest building blocks:

- private owner-only namespace;
- ownership marker and lock;
- stale owned-run cleanup;
- target/draft pair validation;
- stateful MTP exact-boundary semantics;
- temporary files, flush, atomic rename, and directory sync;
- byte-bounded LRU;
- save-failure circuit breaker;
- focused portable failure tests.

Add a **persistent mode** and richer metadata around this engine.

## Target file map

| Target area | Current target files | Recommended work |
|---|---|---|
| Cache configuration | `common/common.h`, `common/arg.cpp`, `tools/server/README.md` | Add explicit mode, retention, durability, namespace, and admin controls. Generate docs/defaults from one schema. |
| Prompt cache engine | `tools/server/server-task.cpp` | Refactor storage policy into a dedicated unit before adding persistent manifests. Preserve current public behavior. |
| State serialization | target `include/llama.h` and server prompt-cache code | Extend metadata for model/state ABI, target/draft/MTP shape, token index, and source/destination sequence IDs. |
| Scheduler/cache scope | `tools/server/server-context.cpp`, task parsing | Introduce authenticated `CacheScope`; unify admission, slot ranking, restore, save, metrics, and release. |
| HTTP/admin | `tools/server/server.cpp` / route layer | Add optional authenticated inspect/prune/invalidate endpoints after core persistence is stable. |
| Tests | `tools/server/tests/unit/test_prompt_cache_disk.py`, `tools/server/tests/utils.py` | Add process-restart, crash-point, tenant, concurrent-instance, migration, and manifest tests. |
| Backend kernels | HIP/Vulkan source | No broad CachyLlama transplant. Diff and benchmark missing narrow Strix cases separately. |
| Packaging | target build/run scripts and notices | Keep MIT source; record imported MIT units; reimplement GPL parent behavior. |

A clean refactor could introduce target-native files such as `server-prompt-cache-storage.*`, `server-prompt-cache-manifest.*`, and `server-cache-scope.*`; names are illustrative, not an existing API.

## Phase 0 — freeze and characterize the target

1. Run the existing disk-cache tests at `a5605a72768c6562241b248e268e33dc92787394`.
2. Capture file format, state_spec/MTP semantics, cleanup, and failure-injection behavior.
3. Add characterization tests before moving code out of `server-task.cpp`.
4. Record exact backend/model test fixtures.

**Exit gate:** no behavior change; all existing tests remain green.

## Phase 1 — refactor storage modes

Introduce:

```text
disabled
run-scoped   # existing semantics and default
persistent   # new explicit behavior
```

The persistent mode reuses the current atomic state-pair writer but does not remove its root on normal shutdown. A mode-specific owner/instance record prevents a run-scoped process from cleaning a persistent namespace.

**Exit gate:** existing CLI retains current cleanup semantics; persistent mode survives a clean restart.

## Phase 2 — versioned manifest and recovery

Manifest minimum fields:

- format and manifest schema version;
- exact model fingerprint and state ABI;
- KV K/V types, context layout, adapter set;
- target/draft/MTP/spec presence and sizes;
- token count, bounded token index/hash, checkpoint reason;
- tenant/conversation scope;
- creation, last access, and logical turn;
- payload filenames, lengths, and checksums;
- publication generation.

Publication sequence:

1. write payload temporary files;
2. validate serialized sizes;
3. flush payload files;
4. rename payloads atomically;
5. write and flush temporary manifest;
6. rename manifest atomically;
7. sync directory;
8. update in-memory index.

Recovery treats the manifest as the visibility point, quarantines orphan/corrupt entries, and has a bounded startup budget.

**Exit gate:** every crash point yields either the old complete entry or the new complete entry, never a partial hit.

## Phase 3 — authenticated cache scope

Create a `CacheScope` with:

```text
model_fingerprint
authenticated_tenant_id  # opaque/keyed; never raw request body identity
conversation_id
reuse_scope              # slot, conversation, tenant-system, process
```

Rules:

- `llama_user_id` and Anthropic `metadata.user_id` are optional hints only.
- An API key/JWT/gateway principal supplies the authoritative tenant.
- Anonymous requests default to process-local reuse and no cross-restart lookup.
- Prefix candidates are filtered for authorization before scoring.
- Raw user IDs never enter paths, logs, or metrics labels.
- Per-tenant quotas and concurrency are enforced by one scheduler registry.

**Exit gate:** adversarial tenant tests prove no cross-scope lookup, timing enumeration, restore, delete, or metrics leakage.

## Phase 4 — continuation and system-prefix classes

### Conversation continuation

- require tenant/model equality;
- require minimum absolute tokens and ratio;
- verify the loaded token vector before applying state;
- preserve destination sequence-ID remap;
- preserve target/draft/MTP exact-boundary rules.

### System-prefix cache

- derive boundary from parsed messages and selected chat template;
- include template identity and relevant template kwargs in the key;
- keep system-prefix scope tenant-local by default;
- allow cross-tenant reuse only for administrator-provisioned immutable public prefixes;
- retain count, byte, and age limits.

**Exit gate:** golden template tests prove that user/tool content is never placed in a reusable system record.

## Phase 5 — unified lifecycle, administration, and observability

One cache registry must own:

- all run-scoped and persistent entries;
- turn completion and tier/age updates;
- RAM/disk accounting;
- tenant/global quotas;
- stats and breaker state;
- model/adaptor invalidation;
- admin list/prune/invalidate operations.

Recommended metrics:

```text
prompt_cache_lookup_total{source,result}
prompt_cache_restore_seconds{source}
prompt_cache_save_total{result}
prompt_cache_bytes{tier}
prompt_cache_entries{tier}
prompt_cache_evictions_total{reason}
prompt_cache_corrupt_total
prompt_cache_breaker_open
prompt_cache_startup_reconcile_seconds
```

No tenant or conversation labels.

**Exit gate:** metrics reconcile with manifest/filesystem accounting under eviction, failure, and restart tests.

## Phase 6 — Strix Halo reconciliation

1. Keep ROCmFPX's current ROCmFPX quant, MTP, HIP, and Vulkan source as baseline.
2. Symbol-diff CachyLlama's gfx115x HIP classification, MMVQ, gated-delta, Vulkan memory accounting, and graph optimizer.
3. Port one narrow change at a time.
4. Run backend-op correctness, model output totals, prompt/decode benchmarks, and cache restore tests.
5. Keep rejected variants and dirty-build provenance separate.

**Exit gate:** no output/correctness regression; accepted change has matched baseline and statistically useful workload results.

## Test matrix

| Dimension | Required cases |
|---|---|
| Process lifecycle | clean stop, SIGTERM, SIGKILL, crash after each publication step, restart during reconciliation |
| Entry shape | target only, target+draft, stateful MTP/spec, hybrid/recurrent, adapter on/off |
| Storage | empty root, full disk, read-only root, short write, rename failure, corrupt/truncated payload, corrupt manifest |
| Matching | exact, shorter prefix, longer prefix, below minimum tokens, below ratio, hash collision simulation, tokenization/template change |
| Scope | same tenant, different tenant, anonymous, changed API principal, changed conversation, public admin prefix |
| Concurrency | two workers same root, lock contention, simultaneous save/load/evict, model router children |
| Upgrade | previous schema, incompatible state ABI, model alias points to new file, rollback |
| Platform | Linux ROCm, Linux Vulkan, supported Windows/macOS server builds where applicable |
| Strix | HIP and Vulkan; dense, MoE, MTP, long context; cold and warm persistent restores |

## Backout

Persistent mode is feature-gated. A production issue can return to `run-scoped` without changing the existing CLI's cleanup semantics. Persistent entries are never automatically imported into run-scoped mode.

## License workstream

- Port only MIT CachyLlama source with attribution.
- Reimplement parent profile/build/deployment behavior without copying GPL code, or distribute it separately under GPL.
- Update ROCmFPX third-party notices and document the exact imported commit.
