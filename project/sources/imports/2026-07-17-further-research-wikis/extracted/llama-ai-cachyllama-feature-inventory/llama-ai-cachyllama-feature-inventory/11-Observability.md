# Observability

Prometheus output, per-response cache/timing data, lifecycle SSE, cache logs, and expert statistics provide useful signals. The target port needs low-cardinality persistent-cache metrics and must fix the pinned omission of user cache aggregation.

| Decision | Count |
|---|---:|
| RETAIN | 3 |
| REDESIGN | 2 |
| REJECT | 1 |

<a id="f-079"></a>
### F-079 — Prometheus metrics endpoint

**Decision:** `RETAIN` · **Portability:** `High` · **Maturity:** `M3` · **Confidence:** `High`

**Observed behavior.** Emits prompt/generation counters, throughput, request queue depth, slot utilization, and process start time.

**Implementation.** `tools/server/server-context.cpp`

**Dependencies.** --metrics; Prometheus text format

**License.** MIT

**ROCmFPX overlap.** ROCmFPX already supports metrics.

**Porting rationale.** Extend target metrics with persistent cache hits/misses/bytes/restore latency/circuit-breaker state.

**Risks / caveats.** Do not label by raw user or conversation.

**Evidence.** [E-092](20-Evidence-Index.md#e-092)

<a id="f-080"></a>
### F-080 — Per-response cache and timing data

**Decision:** `RETAIN` · **Portability:** `High` · **Maturity:** `M3` · **Confidence:** `High`

**Observed behavior.** Returns cached prompt-token counts and prompt/generation timing in completion results.

**Implementation.** `tools/server/server-task.cpp`

**Dependencies.** task result serializer

**License.** MIT

**ROCmFPX overlap.** ROCmFPX tests use cache_n and prompt_n reductions as hit evidence.

**Porting rationale.** Keep target response timings and add an optional cache source field such as ram, run-disk, persistent-disk, or system-prefix.

**Risks / caveats.** Response schema compatibility and information disclosure.

**Evidence.** [E-093](20-Evidence-Index.md#e-093)

<a id="f-081"></a>
### F-081 — SSD cache event logging

**Decision:** `REDESIGN` · **Portability:** `High` · **Maturity:** `M2` · **Confidence:** `High`

**Observed behavior.** Logs checkpoint saves, restores, evictions, cold starts, and failures.

**Implementation.** `common/kv-ssd-cache.cpp`, `tools/server/server-context-page-manager.cpp`, `tools/server/server-context-ssd-cache.cpp`

**Dependencies.** server logger

**License.** MIT

**ROCmFPX overlap.** ROCmFPX already has structured-looking disk cache logs and failure counters.

**Porting rationale.** Normalize target logs with stable event names, byte/token counts, duration, scope pseudonym, and reason codes.

**Risks / caveats.** Raw paths/identifiers can leak sensitive metadata.

**Evidence.** [E-031](20-Evidence-Index.md#e-031), [E-037](20-Evidence-Index.md#e-037), [E-067](20-Evidence-Index.md#e-067)

<a id="f-082"></a>
### F-082 — Model lifecycle SSE

**Decision:** `RETAIN` · **Portability:** `Medium` · **Maturity:** `M2` · **Confidence:** `High`

**Observed behavior.** Broadcasts model load, unload, and reload events to connected clients.

**Implementation.** `tools/server/server-models.cpp`

**Dependencies.** SSE client registry

**License.** MIT

**ROCmFPX overlap.** Useful for orchestration around large ROCmFPX models.

**Porting rationale.** Keep target-native stream with authorization and reconnect semantics.

**Risks / caveats.** Status data can reveal model inventory.

**Evidence.** [E-095](20-Evidence-Index.md#e-095), [E-106](20-Evidence-Index.md#e-106)

<a id="f-083"></a>
### F-083 — Expert/MoE statistics

**Decision:** `REDESIGN` · **Portability:** `Medium` · **Maturity:** `M1` · **Confidence:** `Medium`

**Observed behavior.** Exposes expert-selection information for MoE analysis.

**Implementation.** `tools/server/server.cpp`, `tools/server/server-context.cpp`

**Dependencies.** expert instrumentation

**License.** MIT

**ROCmFPX overlap.** Potentially valuable for Strix Halo MoE kernel tuning.

**Porting rationale.** Validate overhead and output semantics on target models before enabling.

**Risks / caveats.** Sparse evidence and endpoint stability.

**Evidence.** [E-100](20-Evidence-Index.md#e-100)

<a id="f-084"></a>
### F-084 — Cache-stat aggregation

**Decision:** `REJECT` · **Portability:** `High` · **Maturity:** `M1` · **Confidence:** `High`

**Observed behavior.** Internal cache statistics exist, but pinned server aggregation omits user cache maps.

**Implementation.** `common/kv-ssd-cache.h`, `tools/server/server-context-page-manager.cpp`

**Dependencies.** statistics counters; cache registry

**License.** MIT

**ROCmFPX overlap.** ROCmFPX should expose one authoritative cache engine's counters.

**Porting rationale.** Do not copy the split-map aggregation; centralize metrics in the target cache object.

**Risks / caveats.** Undercounting makes capacity and hit-rate decisions unreliable.

**Evidence.** [E-030](20-Evidence-Index.md#e-030), [E-064](20-Evidence-Index.md#e-064)


