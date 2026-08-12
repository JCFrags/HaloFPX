---
section_id: "04"
title: "Ledger Procedures and Checks"
status: "needs-machine-validation"
last_verified: "2026-07-16"
applies_to:
  repositories: ["Custom_Inference_Project"]
  software_versions: []
  hardware_revisions: []
related_sections: ["01", "02", "03", "05"]
---

# Procedures and checks

## Open an assumption or question

Prerequisites: identify the concrete premise/uncertainty and affected work. Root access: not required.

1. Search for duplicate or superseded records.
2. Allocate an immutable ID.
3. State one falsifiable premise or one answerable question.
4. Record impact, owner, confidence, evidence, due condition, validation method, and affected sections.
5. Link the research task or experiment that can resolve it.

## Accept a decision

1. Confirm context, drivers, options, evidence, and unresolved risks.
2. Record selected and rejected alternatives with reasons.
3. Identify decision authority and acceptance timestamp.
4. Link requirements, implementation, experiments, and affected wiki sections.
5. Define consequences, fallback/rollback, and reconsideration trigger.
6. Update dependent records; never rewrite their history silently.

## Closeout review

- Schema and ID validation pass.
- Every evidence link resolves under section [02](../02_Evidence_Citation_and_Source_Policy/README.md).
- Every experiment link resolves under section [05](../05_Research_Data_and_Benchmark_Artifact_Conventions/README.md).
- Status transition is legal and has actor/time/reason.
- Supersession links are reciprocal.
- Answered questions quote or summarize the answer and retain limitations.
- Decisions do not relabel measurements or assumptions as facts.

## Repository exercise

Create disposable candidate records for one assumption, question, task, and ADR. Validate schema, transition each through a normal lifecycle, generate indexes twice, and confirm deterministic identical output. This does not require the two Strix Halo nodes.

## Two-node validation

Use actual topology and matched-build inventory to resolve the illustrative "matched nodes" premise. Store raw environment evidence and link it; do not close the assumption based on identical product names alone.
