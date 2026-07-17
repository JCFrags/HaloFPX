---
section_id: "39"
title: "Coordinator, Rank Worker, Session, and Persistent-Graph Architecture"
status: "needs-machine-validation"
last_verified: "2026-07-17"
applies_to:
  repositories: ["charlie12345/ROCmFPX@a5605a72768c6562241b248e268e33dc92787394", "ggml-org/llama.cpp@788e07dc91d266ad3162a1ce9037665656269689"]
  software_versions: []
  hardware_revisions: ["dual Strix Halo premise"]
related_sections: ["32", "38", "45", "46", "48", "55"]
---

# 39 - Coordinator, Rank Worker, Session, and Persistent-Graph Architecture

**[RECOMMENDATION]** Use one control-plane coordinator and one long-lived inference worker per participating rank. The coordinator owns API semantics, tokenizer, sampler, admission, session directory, mode decisions, and authoritative output; rank workers own device contexts, shard buffers, graphs, rank-local KV/cache, collectives, and health telemetry.

This is a design candidate, not an implementation claim. Current upstream components provide useful lifecycle patterns but no verified HaloFPX coordinator/rank protocol.

**[OPEN]** The recommendation assigns logical sampler authority to the coordinator but does not select where logits reduction or physical sampler/RNG/grammar execution occurs. Rank-local sampling, sampler-state transfer, output commit, replay, and cancellation remain blocked on DR39-O4, DR43-O7, and the state/recovery tests in section 48. No implementation may infer a closed contract from this page.

## Pages

- [Facts and constraints](facts_and_constraints.md)
- [Design implications](design_implications.md)
- [Procedures and checks](procedures_and_checks.md)
- [Open questions](open_questions.md)
- [Sources](sources.md)

## Improvement review

The state-ownership table and handshake make hidden authority explicit. **[OPEN]** Remaining gaps are graph-update support on the selected HIP/runtime stack, exact cache ABI, sampler execution/state handoff, transport integrity, timeout values, and verified shutdown/failure behavior.
