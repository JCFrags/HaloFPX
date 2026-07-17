---
section_id: "04"
title: "Ledger Facts and Constraints"
status: "needs-machine-validation"
last_verified: "2026-07-16"
applies_to:
  repositories: ["Custom_Inference_Project"]
  software_versions: ["MADR HEAD 835fc94baa37887774b1cddddb2ae874881e703b", "JSON Schema 2020-12"]
  hardware_revisions: []
related_sections: ["01", "02", "03", "05"]
---

# Facts and constraints

- **[VERIFIED]** ADR practice defines a record as capturing an important architectural decision with its context and consequences [S04-03].
- **[VERIFIED]** MADR offers full/minimal templates and a numeric `nnnn-title.md` convention [S04-01].
- **[VERIFIED]** JSON Schema Draft 2020-12 can validate typed records and controlled status values [S04-04].
- **[VERIFIED]** W3C PROV includes revision, invalidation, source, entity, activity, and agent concepts useful for traceable ledger transitions [S04-05].
- **[VERIFIED]** Agent Harness review guidance requires accept, revise, defer, or reject decisions grounded in evidence, scope, risk, and dependencies [S04-06].

## Invariants

- **[RECOMMENDATION]** IDs are immutable and never reused; see section [03](../03_Glossary_Naming_and_Stable_Identifiers/README.md).
- **[RECOMMENDATION]** Status transitions append history with actor, UTC time, reason, and evidence. Git history is supporting audit evidence, not a substitute for current state.
- **[RECOMMENDATION]** `owner` is accountable for driving resolution, not proof of approval authority. `decision_authority` is separate.
- **[RECOMMENDATION]** Confidence applies to an assumption/evidence assessment, not to the force of an accepted decision.
- **[RECOMMENDATION]** `due_condition` describes an observable trigger (for example, "before selecting distributed mode"), not only a calendar date.
- **[RECOMMENDATION]** Rejected alternatives remain discoverable with rejection evidence and reconsideration triggers.
- **[RECOMMENDATION]** Supersession is a bidirectional link: old record `superseded_by`, new record `supersedes`.

## Minimum common fields

`id`, `type`, `title`, `status`, `created_at`, `updated_at`, `owner`, `decision_authority`, `confidence`, `statement`, `impact`, `evidence`, `affected_sections`, `related_adrs`, `related_experiments`, `related_issues`, `due_condition`, `validation_method`, `supersedes`, `superseded_by`, and append-only `history`.

**[OPEN]** No actual owner or decision authority is assigned by this research section.
