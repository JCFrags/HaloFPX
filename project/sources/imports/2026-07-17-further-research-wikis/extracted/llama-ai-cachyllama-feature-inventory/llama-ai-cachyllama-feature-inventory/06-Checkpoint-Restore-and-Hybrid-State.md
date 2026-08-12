# Checkpoint restore and hybrid state

The restore path covers target, draft, speculative, manual slot, and hybrid/recurrent state semantics. Destination sequence-ID remapping and safe fallback are central. These semantics align well with ROCmFPX's existing target/draft/MTP validation.

| Decision | Count |
|---|---:|
| RETAIN | 9 |
| REDESIGN | 0 |
| REJECT | 0 |

<a id="f-027"></a>
### F-027 — Periodic prefill checkpoints

**Decision:** `RETAIN` · **Portability:** `High` · **Maturity:** `M2` · **Confidence:** `High`

**Observed behavior.** Creates checkpoints at configured token intervals while processing long prompts.

**Implementation.** `common/common.h`, `common/arg.cpp`, `tools/server/server-context.cpp`

**Dependencies.** llama sequence state API; checkpoint interval; prompt token progress

**License.** MIT

**ROCmFPX overlap.** ROCmFPX already has context checkpoint controls and prompt-cache saves.

**Porting rationale.** Keep target checkpoint scheduling and integrate persistent snapshots as a separate policy.

**Risks / caveats.** Source and documentation disagree on the minimum/default step; very frequent snapshots can dominate I/O.

**Evidence.** [E-022](20-Evidence-Index.md#e-022), [E-023](20-Evidence-Index.md#e-023), [E-071](20-Evidence-Index.md#e-071)

<a id="f-028"></a>
### F-028 — Deferred final checkpoint

**Decision:** `RETAIN` · **Portability:** `Medium` · **Maturity:** `M2` · **Confidence:** `High`

**Observed behavior.** Marks a final checkpoint for creation after the first generated token so the restored state has the needed generation boundary.

**Implementation.** `tools/server/server-context.cpp`

**Dependencies.** slot state machine; generation boundary

**License.** MIT

**ROCmFPX overlap.** ROCmFPX has prompt-cache saves at task boundaries.

**Porting rationale.** Preserve the semantic boundary, but express it as an explicit checkpoint reason/state in the target.

**Risks / caveats.** Ordering bugs can persist a state that cannot resume generation correctly.

**Evidence.** [E-071](20-Evidence-Index.md#e-071)

<a id="f-029"></a>
### F-029 — Target/draft/spec checkpoint restore

**Decision:** `RETAIN` · **Portability:** `High` · **Maturity:** `M2` · **Confidence:** `High`

**Observed behavior.** Restores target state, optional draft state, and speculative state into a selected slot.

**Implementation.** `tools/server/server-context-page-manager.cpp`, `tools/server/server-context-ssd-cache.cpp`

**Dependencies.** llama_state_seq_set_data_ext; draft context; spec state decoder

**License.** MIT

**ROCmFPX overlap.** ROCmFPX already stores target/draft and stateful MTP records.

**Porting rationale.** Merge semantics into the target's paired-state validation and exact-boundary rules.

**Risks / caveats.** A draft/spec mismatch must force safe catch-up or full prefill.

**Evidence.** [E-063](20-Evidence-Index.md#e-063), [E-067](20-Evidence-Index.md#e-067), [E-068](20-Evidence-Index.md#e-068)

<a id="f-030"></a>
### F-030 — Destination sequence-ID remapping

**Decision:** `RETAIN` · **Portability:** `High` · **Maturity:** `M2` · **Confidence:** `High`

**Observed behavior.** Loads a serialized sequence into the currently assigned server slot rather than assuming the source slot ID.

**Implementation.** `include/llama.h`, `tools/server/server-context-page-manager.cpp`, `tools/server/server-context-ssd-cache.cpp`

**Dependencies.** extended sequence state API; destination slot

**License.** MIT

**ROCmFPX overlap.** The target uses per-slot sequence state APIs.

**Porting rationale.** Keep explicit destination sequence IDs in every restore API and add a regression test based on the historical cold-restore fix.

**Risks / caveats.** Implicit source IDs can restore into the wrong sequence.

**Evidence.** [E-063](20-Evidence-Index.md#e-063), [E-068](20-Evidence-Index.md#e-068), [E-070](20-Evidence-Index.md#e-070)

<a id="f-031"></a>
### F-031 — Hybrid attention-only cleanup

**Decision:** `RETAIN` · **Portability:** `High` · **Maturity:** `M2` · **Confidence:** `High`

**Observed behavior.** Clears attention KV state while preserving recurrent state for hybrid/recurrent models.

**Implementation.** `include/llama.h`, `src/llama-memory-hybrid.cpp`, `tools/server/server-context.cpp`

**Dependencies.** hybrid memory implementation; model architecture detection

**License.** MIT

**ROCmFPX overlap.** ROCmFPX includes MTP and evolving recurrent/hybrid support.

**Porting rationale.** Retain the primitive only if the target base does not already contain an equivalent; validate per architecture.

**Risks / caveats.** Incorrect partial clearing can silently corrupt recurrent continuation.

**Evidence.** [E-068](20-Evidence-Index.md#e-068), [E-069](20-Evidence-Index.md#e-069), [E-071](20-Evidence-Index.md#e-071)

<a id="f-032"></a>
### F-032 — Manual slot save/restore/erase

**Decision:** `RETAIN` · **Portability:** `High` · **Maturity:** `M3` · **Confidence:** `High`

**Observed behavior.** Exposes named slot-state files through an HTTP action route when a slot-save path is configured.

**Implementation.** `tools/server/server-context.cpp`, `tools/server/server.cpp`

**Dependencies.** slot save directory; filename validation; HTTP server

**License.** MIT

**ROCmFPX overlap.** ROCmFPX inherits the slot-state API family.

**Porting rationale.** Keep the endpoint but enforce owner-only directories, API authorization, quotas, and audit logs.

**Risks / caveats.** Named files are an administrative surface separate from automatic cache persistence.

**Evidence.** [E-090](20-Evidence-Index.md#e-090), [E-091](20-Evidence-Index.md#e-091)

<a id="f-033"></a>
### F-033 — Cold-restore failure fallback

**Decision:** `RETAIN` · **Portability:** `High` · **Maturity:** `M2` · **Confidence:** `High`

**Observed behavior.** Treats failed or incomplete restores as misses and continues through normal prompt processing.

**Implementation.** `tools/server/server-context-ssd-cache.cpp`, `tools/server/server-context.cpp`

**Dependencies.** restore validation; normal prefill path

**License.** MIT

**ROCmFPX overlap.** ROCmFPX tests corruption and partial target/draft pairs.

**Porting rationale.** Use the target's stricter file-size/pair validation and circuit-breaker semantics.

**Risks / caveats.** Fallback must clear any partially applied sequence state.

**Evidence.** [E-067](20-Evidence-Index.md#e-067), [E-071](20-Evidence-Index.md#e-071)

<a id="f-034"></a>
### F-034 — In-memory prompt state cache

**Decision:** `RETAIN` · **Portability:** `High` · **Maturity:** `M3` · **Confidence:** `High`

**Observed behavior.** Maintains reusable prompt states in RAM in addition to SSD-backed checkpoints.

**Implementation.** `tools/server/server-context.cpp`, `common/common.h`

**Dependencies.** host RAM budget; server prompt cache

**License.** MIT

**ROCmFPX overlap.** ROCmFPX already has a byte-bounded RAM prompt cache.

**Porting rationale.** Keep the target implementation as the first tier; persistent disk should be a lower tier.

**Risks / caveats.** RAM accounting must include target, draft, and metadata.

**Evidence.** [E-022](20-Evidence-Index.md#e-022), [E-071](20-Evidence-Index.md#e-071)

<a id="f-035"></a>
### F-035 — KV-shift cache reuse

**Decision:** `RETAIN` · **Portability:** `Medium` · **Maturity:** `M3` · **Confidence:** `High`

**Observed behavior.** Can reuse sufficiently large prompt chunks via KV shifting under the server cache-reuse setting.

**Implementation.** `common/arg.cpp`, `tools/server/README.md`, `tools/server/server-context.cpp`

**Dependencies.** KV shifting support; prompt cache

**License.** MIT

**ROCmFPX overlap.** ROCmFPX already documents and carries cache-reuse behavior.

**Porting rationale.** Retain target-native behavior; test interactions with loaded persistent state and hybrid memory.

**Risks / caveats.** Not all memory types support arbitrary shifting.

**Evidence.** [E-023](20-Evidence-Index.md#e-023), [E-024](20-Evidence-Index.md#e-024)


