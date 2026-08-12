---
section_id: "58"
title: "Rank-local ownership facts and constraints"
status: "needs-machine-validation"
last_verified: "2026-07-16"
applies_to:
  repositories: ["charlie12345/ROCmFPX", "fewtarius/CachyLLama", "ggml-org/llama.cpp"]
  software_versions: ["ROCmFPX a5605a72768c6562241b248e268e33dc92787394", "CachyLLama 6be745998f568e379ea197fcf827baec73ff9940"]
  hardware_revisions: ["two gfx1151 Strix Halo hosts; exact topology pending"]
related_sections: ["49", "52", "54", "57", "61", "75"]
---

# Rank-local ownership facts and constraints

## Pinned implementation boundary

- **[VERIFIED]** ROCmFPX CLI describes `layer` splitting as layers and KV split across devices (pipelined), and exposes `row`, `tensor`, tensor proportions and RPC devices. [S58-01][S58-02]
- **[VERIFIED]** ROCmFPX sequence-state serialization is implemented through `llama_state_seq_*`/extended APIs owned by one `llama_context`; tensor-split-specific behavior exists in context code but no stable cross-rank checkpoint manifest is exposed. [S58-03]
- **[VERIFIED]** CachyLLama saves/restores target and optional draft state using llama sequence APIs and stores speculative implementation bytes separately. [S58-04]
- **[INFERENCE]** Neither fork proves that a monolithic sequence blob from one plan can be restored under another rank count, split mode, shard map or device order.

## Required ownership model

| Plan | Proposed rank-local state ownership | Restore rule |
|---|---|---|
| Replication | Each rank owns a complete independent state derived from the same verified token prefix, plus rank-local sampler/draft state if it generates. | Either rank may restore locally; never assume byte-identical layout. Cross-rank promotion requires semantic equivalence tests. |
| Pipeline/layer | Rank owns attention/recurrent state for its assigned layer range and plan epoch. Boundary activation state is ephemeral unless section 61 proves it required. | Exact layer-range/topology match; all pipeline stages ready before suffix replay. |
| Tensor parallel | Rank owns the KV/recurrent shard defined by tensor axis, range, layout, world size and collective plan. | Exact world size, shard map, dtype/layout and rank identity match. |
| MoE-aware hybrid | Attention/recurrent state follows layer/tensor plan; expert weights are model artifacts, while any router/expert-history state is persisted only if proven inference-semantic. | Fingerprint expert placement/routing plan and typed state components; do not infer ownership from expert activation statistics. |

**[ASSUMPTION]** HaloFPX initially uses two fixed logical ranks. **[RECOMMENDATION]** Rank identity must be logical and bound to a topology-plan digest; hostname/device enumeration order is diagnostic metadata, not object identity.

## Hard constraints

- **[RECOMMENDATION]** Checkpoint ID = compatibility fingerprint + topology-plan digest + token-prefix digest + generation + component manifest digest. Section 57 defines canonical encoding/hash.
- **[RECOMMENDATION]** A rank manifest names rank ID, owned layer/tensor/expert ranges, token interval, component type/schema/digest/length and local object key.
- **[RECOMMENDATION]** Readiness messages contain IDs/digests/status only. Normal restore must not send state pages over USB4.
- **[RECOMMENDATION]** A rank may publish staged state into a live slot only after every required rank reports the same checkpoint/generation/plan and the coordinator issues commit-to-live.
- **[RECOMMENDATION]** Rank loss during a collective invalidates the distributed attempt. Partial output/state is never consumed.
- **[OPEN]** Whether any topology transition can safely reuse a prefix by reshaping state rather than recomputation is unproven and must default to miss.

