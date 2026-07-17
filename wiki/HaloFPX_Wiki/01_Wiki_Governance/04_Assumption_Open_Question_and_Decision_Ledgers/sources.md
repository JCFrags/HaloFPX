---
section_id: "04"
title: "Ledger Sources"
status: "needs-machine-validation"
last_verified: "2026-07-16"
applies_to:
  repositories: ["Custom_Inference_Project", "Agent_Harness"]
  software_versions: ["MADR HEAD 835fc94baa37887774b1cddddb2ae874881e703b", "JSON Schema 2020-12"]
  hardware_revisions: []
related_sections: ["02"]
---

# Sources

| ID | Source and revision | Location | Supports | Limitations |
|---|---|---|---|---|
| S04-01 | MADR repository, HEAD `835fc94baa37887774b1cddddb2ae874881e703b` observed 2026-07-16 | https://github.com/adr/madr/tree/835fc94baa37887774b1cddddb2ae874881e703b | ADR templates and numeric naming | Project chooses its own fields/statuses |
| S04-02 | Agent Harness improvement proposal template, accessed 2026-07-16 | `C:/Users/britt/Documents/Agent_Harness/templates/improvement-proposal.md` | Evidence/risk/validation fields | Local template, not an industry standard |
| S04-03 | Architecture Decision Record repository overview, commit `b7654d64ecbcfaf8bce04678b568ea3483c91a94` | https://github.com/architecture-decision-record/architecture-decision-record/tree/b7654d64ecbcfaf8bce04678b568ea3483c91a94 | ADR/ADL definitions | Curated community primary project |
| S04-04 | JSON Schema Draft 2020-12, published 2022-06-16 | https://json-schema.org/draft/2020-12 | Machine validation | Does not prescribe ledger semantics |
| S04-05 | W3C PROV-O Recommendation, 2013-04-30 | https://www.w3.org/TR/2013/REC-prov-o-20130430/ | Revision/invalidation/provenance | Simplified mapping here |
| S04-06 | Agent Harness review instructions, accessed 2026-07-16 | `C:/Users/britt/Documents/Agent_Harness/reviews/AGENTS.md` | Review outcomes and evidence gate | Local authority |
| S04-07 | RFC 3339, 2002-07 | https://www.rfc-editor.org/rfc/rfc3339 | UTC timestamp format | Does not define lifecycle |

Access date for web sources: 2026-07-16. Ledger schemas and status transitions are HaloFPX recommendations, not externally verified requirements.
