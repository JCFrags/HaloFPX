---
section_id: "15"
title: "Integration open questions"
status: "needs-machine-validation"
last_verified: "2026-07-16"
applies_to:
  repositories:
    - "HaloFPX integration repository (proposed)"
    - "ggml-org/llama.cpp@788e07dc91d266ad3162a1ce9037665656269689"
    - "charlie12345/ROCmFPX@a5605a72768c6562241b248e268e33dc92787394"
    - "fewtarius/CachyLLama@6be745998f568e379ea197fcf827baec73ff9940"
    - "fewtarius/llama-ai@1017f3dfdce3ca2b06aa9007b23295db3bb35722"
  software_versions: []
  hardware_revisions: ["two matched AMD Strix Halo systems; exact revisions open"]
related_sections: ["11", "13", "14", "16"]
---

# Integration open questions

| ID | Question | Evidence/owner needed | Blocks |
|---|---|---|---|
| `OQ-15-01` | **[OPEN]** What exact llama.cpp commit best represents the source snapshot from which current ROCmFPX files were assembled? | Section 11 lineage analysis; tree similarity plus author/source records | One-time ancestry normalization |
| `OQ-15-02` | **[OPEN]** Which ROCmFPX commits/paths are required, optional, generated, or already upstream? | Section 13 commit-to-feature inventory at `a5605a7...` | Lane 10 manifest and bisect units |
| `OQ-15-03` | **[OPEN]** Which CachyLLama commits implement required persistent-cache semantics without importing unrelated server policy? | Section 14 semantic/commit inventory at `6be7459...` | Lane 20 scope |
| `OQ-15-04` | **[OPEN]** Does any desired implementation code come only from GPL-licensed llama-ai rather than MIT CachyLLama? | Section 16 file-level license review and counsel if necessary | Core license and process boundary |
| `OQ-15-05` | **[OPEN]** What is the HaloKV cache ABI key and invalidation/migration policy across runtime, model, tokenizer, backend, rank, and topology changes? | Cache design plus `EXP-15-03` and `EXP-15-06` | Safe persistence and rollback |
| `OQ-15-06` | **[OPEN]** Which process owns rank/session metadata, cancellation, and recovery after one link or rank fails? | Distributed architecture decision plus `EXP-15-04` | Lane 30/40 interface |
| `OQ-15-07` | **[OPEN]** Should Halo fabric and HaloKV be core patches, linked libraries, or process-separated services? | Conflict-frequency estimate, latency data, failure isolation, license review | Long-term fork surface |
| `OQ-15-08` | **[OPEN]** What CI subset is fast enough per commit and what two-machine matrix is mandatory per candidate? | Section 16 CI design and `EXP-15-01` timings | Branch protection and cadence |
| `OQ-15-09` | **[OPEN]** What upstream support window and synchronization service-level objective are sustainable? | At least two rehearsed sync cycles with conflict and machine-hour records | Release policy |
| `OQ-15-10` | **[OPEN]** Which patches have maintainers willing to own upstream submissions and long-term review? | Named owners and upstream issue/PR discussion | Fork reduction plan |
| `OQ-15-11` | **[OPEN]** What constitutes acceptable single-node fallback: automatic retry, explicit operator action, or a separate endpoint? | Product safety decision and failure-injection evidence | User-visible failure semantics |
| `OQ-15-12` | **[OPEN]** Where will source snapshots, range-diffs, conflict records, machine evidence, and signed manifests be preserved? | Repository/evidence retention decision | Reproducibility and auditability |

## Resolution rule

An answer is not closed merely by choosing an option. Record the decision, exact evidence, affected patch lanes, owner, compatibility scope, rollback, and date. Machine-dependent answers require linked raw experiment data and environment metadata; repository claims require exact commits.

## Follow-up order

1. Complete sections 11, 13, 14, and 16 at exact pinned commits.
2. Resolve `OQ-15-01` through `OQ-15-04` before writing the first combined patch.
3. Specify cache/fabric contracts for `OQ-15-05` through `OQ-15-07` before persistence or distributed execution is enabled.
4. Run `EXP-15-01` through `EXP-15-06`; use results to resolve cadence and release questions.
5. Review remaining questions at every candidate closeout.
