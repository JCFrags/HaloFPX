# Cache administration

Configuration, retention, startup discovery, eviction, invalidation, and statistics are spread across the component and parent runner. ROCmFPX should centralize them under one cache engine and a generated configuration schema.

| Decision | Count |
|---|---:|
| RETAIN | 2 |
| REDESIGN | 6 |
| REJECT | 1 |

<a id="f-048"></a>
### F-048 — SSD cache CLI and environment controls

**Decision:** `REDESIGN` · **Portability:** `High` · **Maturity:** `M2` · **Confidence:** `High`

**Observed behavior.** Exposes path, tier windows/budgets, checkpoint limits, system-prompt limits, retention, fsync, and concurrency settings.

**Implementation.** `common/common.h`, `common/arg.cpp`, `tools/server/README.md`

**Dependencies.** CLI parser; environment variables

**License.** MIT

**ROCmFPX overlap.** ROCmFPX currently exposes --cache-disk and --cache-disk-limit.

**Porting rationale.** Extend target-native naming with explicit persistent-mode, durability, retention, and namespace controls; preserve backward compatibility.

**Risks / caveats.** Cachy source, docs, and parent runner use conflicting defaults.

**Evidence.** [E-022](20-Evidence-Index.md#e-022), [E-023](20-Evidence-Index.md#e-023), [E-024](20-Evidence-Index.md#e-024)

<a id="f-049"></a>
### F-049 — Parent profile overrides

**Decision:** `REDESIGN` · **Portability:** `High` · **Maturity:** `M2` · **Confidence:** `High`

**Observed behavior.** llama-ai supplies operational defaults that differ from component defaults and selects cache use by model/hardware profile.

**Implementation.** `llama-run.sh`

**Dependencies.** shell environment; model metadata probe; hardware detector

**License.** GPL-3.0

**ROCmFPX overlap.** ROCmFPX has build/run scripts but should not directly import GPL shell code into its MIT tree.

**Porting rationale.** Re-express useful profile rules in an MIT-compatible configuration layer or external GPL deployment project.

**Risks / caveats.** License incompatibility for direct incorporation; default drift.

**Evidence.** [E-005](20-Evidence-Index.md#e-005), [E-006](20-Evidence-Index.md#e-006), [E-007](20-Evidence-Index.md#e-007), [E-118](20-Evidence-Index.md#e-118)

<a id="f-050"></a>
### F-050 — Retention by unused days

**Decision:** `RETAIN` · **Portability:** `High` · **Maturity:** `M2` · **Confidence:** `High`

**Observed behavior.** Prunes cache entries not used within a configured number of days.

**Implementation.** `common/kv-ssd-system-cache.cpp`, `common/kv-ssd-cache.cpp`

**Dependencies.** wall clock; last-access metadata

**License.** MIT

**ROCmFPX overlap.** ROCmFPX currently evicts by byte pressure and run lifetime.

**Porting rationale.** Add optional age eviction to persistent mode with monotonic/validated timestamps and dry-run reporting.

**Risks / caveats.** Clock rollback and unreadable metadata.

**Evidence.** [E-037](20-Evidence-Index.md#e-037), [E-053](20-Evidence-Index.md#e-053)

<a id="f-051"></a>
### F-051 — Conversation-count eviction

**Decision:** `REDESIGN` · **Portability:** `High` · **Maturity:** `M1` · **Confidence:** `High`

**Observed behavior.** Deletes an old conversation cache directory when the configured conversation limit is reached.

**Implementation.** `tools/server/server-context-page-manager.cpp`

**Dependencies.** directory mtimes; filesystem removal

**License.** MIT

**ROCmFPX overlap.** ROCmFPX uses byte-bounded LRU entries.

**Porting rationale.** Use manifest-backed LRU/bytes per tenant; do not choose victims solely by directory mtime.

**Risks / caveats.** Separate user/anonymous limits and non-atomic deletion.

**Evidence.** [E-065](20-Evidence-Index.md#e-065)

<a id="f-052"></a>
### F-052 — Startup discovery and cleanup

**Decision:** `REDESIGN` · **Portability:** `High` · **Maturity:** `M2` · **Confidence:** `High`

**Observed behavior.** Discovers existing persistent entries at startup; malformed or expired entries can be skipped or removed.

**Implementation.** `common/kv-ssd-cache.cpp`, `common/kv-ssd-system-cache.cpp`

**Dependencies.** directory scan; metadata parser

**License.** MIT

**ROCmFPX overlap.** ROCmFPX already cleans stale private run directories using ownership markers and locks.

**Porting rationale.** Combine target ownership/lock safety with a persistent manifest, quarantine, bounded reconciliation, and schema migration.

**Risks / caveats.** Concurrent instances and partial entries need clear ownership.

**Evidence.** [E-031](20-Evidence-Index.md#e-031), [E-051](20-Evidence-Index.md#e-051), [E-053](20-Evidence-Index.md#e-053)

<a id="f-053"></a>
### F-053 — Administrative cache API

**Decision:** `REDESIGN` · **Portability:** `Medium` · **Maturity:** `M1` · **Confidence:** `Medium`

**Observed behavior.** No dedicated HTTP endpoint for listing, inspecting, pruning, or invalidating automatic SSD cache entries was identified.

**Implementation.** `tools/server/server.cpp`, `tools/server/server-context.cpp`

**Dependencies.** HTTP route table

**License.** MIT

**ROCmFPX overlap.** ROCmFPX likewise exposes metrics/slots rather than a persistent-cache admin API.

**Porting rationale.** Add authenticated inspect/prune/invalidate endpoints or an offline admin CLI, with tenant scoping and dry-run.

**Risks / caveats.** Administrative APIs can expose prompt metadata and become a destructive control surface.

**Evidence.** [E-090](20-Evidence-Index.md#e-090), [E-091](20-Evidence-Index.md#e-091)

<a id="f-054"></a>
### F-054 — Cache statistics API surface

**Decision:** `REDESIGN` · **Portability:** `High` · **Maturity:** `M1` · **Confidence:** `High`

**Observed behavior.** Cache internals track hits, misses, bytes, tiers, and restore timing, but complete user-scoped aggregation is not wired into the standard metrics endpoint.

**Implementation.** `common/kv-ssd-cache.h`, `tools/server/server-context-page-manager.cpp`, `tools/server/server-context.cpp`

**Dependencies.** cache stats; Prometheus renderer

**License.** MIT

**ROCmFPX overlap.** ROCmFPX already tracks disk saves, loads, evictions, failures, and prompt cache hits in logs/timings.

**Porting rationale.** Expose low-cardinality cache counters and gauges from one target cache engine.

**Risks / caveats.** Metrics cardinality and undercounting.

**Evidence.** [E-030](20-Evidence-Index.md#e-030), [E-064](20-Evidence-Index.md#e-064), [E-092](20-Evidence-Index.md#e-092), [E-093](20-Evidence-Index.md#e-093)

<a id="f-055"></a>
### F-055 — Cache invalidation on compatibility change

**Decision:** `RETAIN` · **Portability:** `High` · **Maturity:** `M2` · **Confidence:** `High`

**Observed behavior.** Compatibility checks cause old state to be rejected when model/cache identity changes.

**Implementation.** `common/kv-ssd-cache.cpp`, `common/kv-ssd-system-cache.cpp`

**Dependencies.** compatibility fingerprint

**License.** MIT

**ROCmFPX overlap.** ROCmFPX model formats and MTP state evolve rapidly.

**Porting rationale.** Retain fail-closed rejection and add explicit invalidate/migrate tooling keyed by exact model fingerprint.

**Risks / caveats.** Rejected files can consume disk indefinitely without cleanup.

**Evidence.** [E-038](20-Evidence-Index.md#e-038), [E-052](20-Evidence-Index.md#e-052)

<a id="f-056"></a>
### F-056 — Standalone asynchronous KV page-manager prototype

**Decision:** `REJECT` · **Portability:** `High` · **Maturity:** `M0` · **Confidence:** `High`

**Observed behavior.** A second hot/warm/cold page-manager implementation includes asynchronous writeback concepts but is not linked into the active common library.

**Implementation.** `common/kv_page_manager.h`, `common/kv_page_manager.cpp`, `test_kv_page_manager.cpp`, `common/CMakeLists.txt`

**Dependencies.** worker thread; page table; filesystem

**License.** MIT

**ROCmFPX overlap.** ROCmFPX already has a working prompt cache engine.

**Porting rationale.** Do not port this duplicate path. Extract only independently valuable test ideas after verifying semantics.

**Risks / caveats.** Unbuilt code, inconsistent size comments/literals, duplicate architecture.

**Evidence.** [E-072](20-Evidence-Index.md#e-072), [E-073](20-Evidence-Index.md#e-073), [E-074](20-Evidence-Index.md#e-074), [E-075](20-Evidence-Index.md#e-075)


