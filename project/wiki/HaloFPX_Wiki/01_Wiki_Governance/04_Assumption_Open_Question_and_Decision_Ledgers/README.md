---
section_id: "04"
title: "Assumption, Open-Question, and Decision Ledgers"
status: "needs-machine-validation"
last_verified: "2026-07-16"
applies_to:
  repositories: ["Custom_Inference_Project"]
  software_versions: ["MADR HEAD 835fc94baa37887774b1cddddb2ae874881e703b"]
  hardware_revisions: []
related_sections: ["01", "02", "03", "05", "43", "49"]
---

# Assumption, Open-Question, and Decision Ledgers

**[RECOMMENDATION]** Maintain separate typed records for assumptions, open questions, research tasks, and decisions, while exposing a generated unified index. A decision is not an assumption with a stronger tone, and an answered question is not automatically an accepted design choice.

## Ledger roles

| Record | Purpose | Terminal handling |
|---|---|---|
| Assumption | Unproven premise currently used for planning | validate, invalidate, or supersede |
| Open question | Specific unresolved uncertainty | answer with evidence or close as no-longer-relevant |
| Research task | Bounded work to obtain evidence | complete, cancel, or supersede |
| ADR | Significant accepted/rejected choice with context and consequences | accept, reject, deprecate, or supersede; never rewrite history silently |

**[VERIFIED]** MADR provides Markdown templates centered on decisions, context, options, and consequences and uses numbered decision filenames [S04-01]. **[VERIFIED]** Agent Harness requires improvement proposals to state evidence, benefit, regression risk, validation, and pending decision [S04-02].

## Research split

- Source research completed: ADR structure, machine-validatable YAML, provenance/revision relations, and Agent Harness review gates.
- Repository/machine work required: populate real assumptions and questions from hardware/runtime inspections; exercise lifecycle transitions and link validation.
- Contingent decision: ledger storage layout, approval authority, due-condition automation, and issue-tracker integration.

See [facts](facts_and_constraints.md), [schema implications](design_implications.md), [procedures](procedures_and_checks.md), [questions](open_questions.md), and [sources](sources.md).
