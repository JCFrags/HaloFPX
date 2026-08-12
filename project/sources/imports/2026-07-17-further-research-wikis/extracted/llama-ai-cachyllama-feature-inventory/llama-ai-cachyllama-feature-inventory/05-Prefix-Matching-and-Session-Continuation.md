# Prefix matching and session continuation

The component uses longest-common-prefix scoring for checkpoint reuse and a higher threshold for cross-conversation continuation. Cross-restart and cross-conversation reuse must be partitioned by model and authenticated tenant and must combine an absolute token minimum with a ratio.

| Decision | Count |
|---|---:|
| RETAIN | 4 |
| REDESIGN | 2 |
| REJECT | 0 |

<a id="f-021"></a>
### F-021 — Longest-common-prefix candidate scoring

**Decision:** `RETAIN` · **Portability:** `High` · **Maturity:** `M2` · **Confidence:** `High`

**Observed behavior.** Compares incoming tokens with stored checkpoint prefixes and ranks by longest common prefix, recency, and checkpoint length.

**Implementation.** `common/kv-ssd-cache.cpp`

**Dependencies.** stored token prefixes; token equality

**License.** MIT

**ROCmFPX overlap.** ROCmFPX already performs LCP matching for RAM and disk prompt cache entries.

**Porting rationale.** Consolidate around the target implementation and add restart-persistent index candidates.

**Risks / caveats.** Minimum acceptance and exact state boundary rules must remain caller-specific.

**Evidence.** [E-035](20-Evidence-Index.md#e-035)

<a id="f-022"></a>
### F-022 — High-threshold cross-conversation continuation

**Decision:** `REDESIGN` · **Portability:** `Medium` · **Maturity:** `M2` · **Confidence:** `High`

**Observed behavior.** Requires approximately 90% token-prefix similarity before restoring another conversation's checkpoint.

**Implementation.** `tools/server/server-context-page-manager.cpp`

**Dependencies.** LCP score; conversation index

**License.** MIT

**ROCmFPX overlap.** ROCmFPX prompt caching already matches prefixes within the running server.

**Porting rationale.** Make threshold configurable by scope; require authenticated tenant equality and a minimum absolute token count.

**Risks / caveats.** A ratio alone is unsafe for short prompts.

**Evidence.** [E-062](20-Evidence-Index.md#e-062)

<a id="f-023"></a>
### F-023 — Global anonymous continuation scan

**Decision:** `REDESIGN` · **Portability:** `High` · **Maturity:** `M1` · **Confidence:** `High`

**Observed behavior.** Searches anonymous cache directories for a stored prompt that can continue a new conversation.

**Implementation.** `common/kv-ssd-cache.cpp`, `tools/server/server-context-page-manager.cpp`

**Dependencies.** directory enumeration; token-prefix metadata

**License.** MIT

**ROCmFPX overlap.** ROCmFPX does not retain cross-run directories.

**Porting rationale.** Replace global scanning with a manifest/index partitioned by model and authenticated tenant; anonymous reuse should default to process-local only.

**Risks / caveats.** Cross-user data reuse and unbounded I/O.

**Evidence.** [E-036](20-Evidence-Index.md#e-036), [E-062](20-Evidence-Index.md#e-062)

<a id="f-024"></a>
### F-024 — Minimum compared-token guard

**Decision:** `RETAIN` · **Portability:** `Medium` · **Maturity:** `M2` · **Confidence:** `High`

**Observed behavior.** Rejects very small continuation comparisons before treating them as a useful session match.

**Implementation.** `common/kv-ssd-cache.cpp`

**Dependencies.** token count

**License.** MIT

**ROCmFPX overlap.** ROCmFPX can apply this in its candidate filter.

**Porting rationale.** Use both minimum absolute tokens and ratio thresholds, with higher requirements for cross-conversation or cross-restart matches.

**Risks / caveats.** Fixed thresholds may be model/template dependent.

**Evidence.** [E-036](20-Evidence-Index.md#e-036)

<a id="f-025"></a>
### F-025 — Slot prompt similarity

**Decision:** `RETAIN` · **Portability:** `High` · **Maturity:** `M3` · **Confidence:** `High`

**Observed behavior.** Uses a configurable slot-prompt similarity threshold to choose a warm slot with a reusable prefix.

**Implementation.** `tools/server/server-context.cpp`, `common/arg.cpp`, `llama-run.sh`

**Dependencies.** live slot prompts; LCP score

**License.** MIT for component; GPL-3.0 for parent profile

**ROCmFPX overlap.** ROCmFPX inherits llama.cpp slot prompt reuse.

**Porting rationale.** Keep target behavior; document interaction with persistent cache and user affinity.

**Risks / caveats.** Affinity and prefix reuse must be evaluated under one authorization-aware selection policy.

**Evidence.** [E-005](20-Evidence-Index.md#e-005), [E-023](20-Evidence-Index.md#e-023), [E-082](20-Evidence-Index.md#e-082)

<a id="f-026"></a>
### F-026 — Read-ahead of likely continuation

**Decision:** `RETAIN` · **Portability:** `Medium` · **Maturity:** `M2` · **Confidence:** `High`

**Observed behavior.** Prefetches the selected checkpoint so restore latency can overlap request preparation.

**Implementation.** `common/kv-ssd-cache.cpp`, `tools/server/server-context-page-manager.cpp`

**Dependencies.** candidate prediction; OS page cache

**License.** MIT

**ROCmFPX overlap.** No target equivalent was identified.

**Porting rationale.** Add only after persistence and candidate correctness are tested; instrument useful-prefetch ratio.

**Risks / caveats.** Can amplify SSD writes/reads and page-cache churn.

**Evidence.** [E-042](20-Evidence-Index.md#e-042), [E-062](20-Evidence-Index.md#e-062)


