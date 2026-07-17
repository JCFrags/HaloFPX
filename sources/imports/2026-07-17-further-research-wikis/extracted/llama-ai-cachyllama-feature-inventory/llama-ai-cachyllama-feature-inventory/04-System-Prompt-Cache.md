# System-prompt cache

A dedicated system-prefix catalog enables reuse across conversations and restarts. Hashing, compatibility checks, retention, and bounded entry count are useful. The pinned boundary detector is heuristic and must be rejected in favor of structural message/template boundaries.

| Decision | Count |
|---|---:|
| RETAIN | 2 |
| REDESIGN | 2 |
| REJECT | 1 |

<a id="f-016"></a>
### F-016 — Cross-conversation system-state reuse

**Decision:** `REDESIGN` · **Portability:** `High` · **Maturity:** `M2` · **Confidence:** `High`

**Observed behavior.** Stores a system-prefix sequence state independently of a conversation so requests sharing the same system prompt can skip repeated prefill.

**Implementation.** `common/kv-ssd-system-cache.h`, `common/kv-ssd-system-cache.cpp`, `tools/server/server-context.cpp`

**Dependencies.** system boundary detector; sequence-state API; model compatibility hash

**License.** MIT

**ROCmFPX overlap.** ROCmFPX has general prompt-prefix caching but no separately governed persistent system-prefix store.

**Porting rationale.** Implement as a scoped prefix-cache class in the target engine, keyed by model/template/tenant/system token sequence.

**Risks / caveats.** Cross-tenant leakage and false boundary detection are high-impact.

**Evidence.** [E-050](20-Evidence-Index.md#e-050), [E-052](20-Evidence-Index.md#e-052), [E-055](20-Evidence-Index.md#e-055)

<a id="f-017"></a>
### F-017 — Restart persistence for system prefixes

**Decision:** `REDESIGN` · **Portability:** `High` · **Maturity:** `M2` · **Confidence:** `High`

**Observed behavior.** Scans sys-{hash}.bin entries and restores their state into an in-memory catalog at startup.

**Implementation.** `common/kv-ssd-system-cache.cpp`

**Dependencies.** persistent files; startup scan; RAM catalog

**License.** MIT

**ROCmFPX overlap.** ROCmFPX's cache is run-scoped.

**Porting rationale.** Use the same versioned persistent manifest and atomic commit layer as conversation checkpoints.

**Risks / caveats.** Startup memory cost scales with stored system entries because state blobs are loaded into RAM.

**Evidence.** [E-051](20-Evidence-Index.md#e-051)

<a id="f-018"></a>
### F-018 — Entry-count and age retention

**Decision:** `RETAIN` · **Portability:** `High` · **Maturity:** `M2` · **Confidence:** `High`

**Observed behavior.** Expires system cache entries after a configured unused-day limit and evicts LRU entries above a maximum count.

**Implementation.** `common/kv-ssd-system-cache.h`, `common/kv-ssd-system-cache.cpp`

**Dependencies.** wall clock; LRU metadata; filesystem delete

**License.** MIT

**ROCmFPX overlap.** ROCmFPX already has byte-bounded LRU mechanics.

**Porting rationale.** Combine age, byte, and count limits; expose tenant and global quotas.

**Risks / caveats.** Wall-clock changes and large state variance require conservative accounting.

**Evidence.** [E-050](20-Evidence-Index.md#e-050), [E-053](20-Evidence-Index.md#e-053)

<a id="f-019"></a>
### F-019 — Full-prompt hash plus bounded token prefix

**Decision:** `RETAIN` · **Portability:** `High` · **Maturity:** `M2` · **Confidence:** `High`

**Observed behavior.** Keys a system entry by a full prompt hash and records a bounded token prefix and compatibility metadata.

**Implementation.** `common/kv-ssd-system-cache.h`, `common/kv-ssd-system-cache.cpp`

**Dependencies.** stable tokenization; hash function; compatibility fingerprint

**License.** MIT

**ROCmFPX overlap.** ROCmFPX can reuse its token vectors and target/draft validation.

**Porting rationale.** Use a cryptographic or collision-resistant content key over canonical tokens and include template/model/tenant scope.

**Risks / caveats.** Hash collisions or tokenization drift must fail closed.

**Evidence.** [E-050](20-Evidence-Index.md#e-050), [E-052](20-Evidence-Index.md#e-052)

<a id="f-020"></a>
### F-020 — Heuristic system-boundary detection

**Decision:** `REJECT` · **Portability:** `High` · **Maturity:** `M1` · **Confidence:** `High`

**Observed behavior.** Decodes token text and searches for user/human/end-of-generation markers to decide where the reusable system prefix ends.

**Implementation.** `common/kv-ssd-system-cache.cpp`

**Dependencies.** token decoder; chat template conventions

**License.** MIT

**ROCmFPX overlap.** ROCmFPX supports multiple chat templates and structured request conversion.

**Porting rationale.** Derive the boundary from the parsed message/template AST before tokenization, then verify the exact token range.

**Risks / caveats.** Heuristics can cache user content as a shared system prefix.

**Evidence.** [E-054](20-Evidence-Index.md#e-054)


