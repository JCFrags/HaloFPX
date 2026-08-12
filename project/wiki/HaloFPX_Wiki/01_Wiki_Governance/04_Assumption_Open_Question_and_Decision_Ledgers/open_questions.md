---
section_id: "04"
title: "Ledger Open Questions"
status: "needs-machine-validation"
last_verified: "2026-07-16"
applies_to:
  repositories: ["Custom_Inference_Project"]
  software_versions: []
  hardware_revisions: []
related_sections: ["01", "02", "05"]
---

# Open questions

| ID | Question | Needed evidence | Impact |
|---|---|---|---|
| OQ-04-001 | Who can accept project ADRs? | User/team governance decision | Prevents accidental authority |
| OQ-04-002 | Are ledgers Markdown-first, YAML-first, or dual generated views? | Retrieval/editing prototype | Maintenance and validation |
| OQ-04-003 | Which status transitions require review? | Risk classification | Workflow friction vs safety |
| OQ-04-004 | How are external issue IDs namespaced across hosts? | Hosting decision | Stable cross-links |
| OQ-04-005 | Should calendar review dates supplement due conditions? | Maintenance trial | Staleness detection |
| OQ-04-006 | What blocks implementation: any open question, or only `blocking: true`? | Governance decision | Automation semantics |
| OQ-04-007 | Where are rejected experiments and alternatives retained? | Artifact retention decision | Relearning prevention |

**[OPEN]** The project currently has wiki requirements for ledgers but this section did not create root ledger directories or assign owners because the output scope is limited to this wiki section.

## Follow-up research

- Prototype schema-driven lifecycle validation.
- Map all 86 section prompts to initial assumptions/questions without promoting prompt premises into decisions.
- Test generated indexes for retrieval usefulness with Codex.
