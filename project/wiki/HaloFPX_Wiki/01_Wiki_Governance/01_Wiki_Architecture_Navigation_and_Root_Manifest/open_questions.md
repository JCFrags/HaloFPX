---
section_id: "01"
title: "Wiki Architecture Open Questions"
status: "needs-machine-validation"
last_verified: "2026-07-16"
applies_to:
  repositories: ["Custom_Inference_Project"]
  software_versions: []
  hardware_revisions: []
related_sections: ["02", "03", "04", "05"]
---

# Open questions

| ID | Question | Resolution evidence | Impact |
|---|---|---|---|
| OQ-01-001 | Who owns each category and the root manifest? | Named maintainers and review workflow | Required before CODEOWNERS enforcement |
| OQ-01-002 | Should root/category README files be generated and committed? | Determinism test and contributor usability review | Affects drift risk |
| OQ-01-003 | Which validator runtime exists on Windows and both nodes? | Host inventory | Affects CI and local checks |
| OQ-01-004 | What constitutes a unique `authoritative_for` topic? | Trial registry plus collision review | Prevents duplicate pages |
| OQ-01-005 | How long should archived sections remain in ordinary clones? | Repository-size and audit requirements | Retention policy |

**[OPEN]** No two-node measurements are required to establish the conceptual information architecture, but cross-platform path/link validation is required before marking the section verified.

## Follow-up research

- Pin the exact validator dependencies and their licenses.
- Evaluate whether a Software Heritage identifier is useful in addition to Git commit IDs for upstream source durability.
- Define migration behavior if the global section ID width exceeds two digits.
