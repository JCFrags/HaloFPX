---
section_id: "02"
title: "Evidence Policy Open Questions"
status: "needs-machine-validation"
last_verified: "2026-07-16"
applies_to:
  repositories: ["Custom_Inference_Project"]
  software_versions: []
  hardware_revisions: []
related_sections: ["04", "05"]
---

# Open questions

| ID | Question | Needed evidence | Impact |
|---|---|---|---|
| OQ-02-001 | Which upstream commits will the project pin? | Architecture decision plus build validation | Applicability of implementation claims |
| OQ-02-002 | Must every web source be preserved locally? | License/storage/risk review | Durability and repository size |
| OQ-02-003 | What review triggers and maximum ages apply by source class? | Change-rate study | Freshness automation |
| OQ-02-004 | Should claims live in separate machine-readable files or Markdown only? | Retrieval and maintenance prototype | Validator complexity |
| OQ-02-005 | Is SHA-256 sufficient for all artifact classes, or should SWHID/DOI be added? | Archival requirements | Long-term identity |
| OQ-02-006 | What evidence is required to promote a repository claim to verified local behavior? | Experiment policy decisions by feature class | Prevents overclaiming |

**[OPEN]** The observed upstream HEADs are volatile and are not selections. Re-check them when an ADR proposes a pin.

## Follow-up research

- Review repository licenses and third-party notices at the selected commits.
- Define a claim-schema validator and test it against contradictory version-scoped facts.
- Decide whether offline source bundles are needed for disaster recovery.
