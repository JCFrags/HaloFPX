---
section_id: "09"
title: "Requirements and SLO Open Questions"
status: "needs-machine-validation"
last_verified: "2026-07-16"
applies_to:
  repositories: ["HaloFPX project"]
  software_versions: []
  hardware_revisions: []
related_sections: ["06", "07", "08", "38", "48", "69", "71"]
---

# Open questions

| ID | Question | Resolution |
|---|---|---|
| OQ-09-01 | **[OPEN]** Who ratifies requirements and waivers? | Named owner and governance rule |
| OQ-09-02 | **[OPEN]** Which API endpoints/options are mandatory? | Client conformance inventory |
| OQ-09-03 | **[OPEN]** Which workload cells and model profiles define release acceptance? | Section 07 trace plus model catalog |
| OQ-09-04 | **[OPEN]** Are the candidate SLO multipliers and sample counts acceptable? | Baselines and sponsor decision |
| OQ-09-05 | **[OPEN]** What task-quality metrics and tolerances apply per model? | Versioned evaluation plan |
| OQ-09-06 | **[OPEN]** What maintenance window and mature availability target apply? | Operations policy |
| OQ-09-07 | **[OPEN]** Which failures permit automatic retry versus explicit client failure? | Idempotency and session semantics ADR |
| OQ-09-08 | **[OPEN]** What cache compatibility fields and cryptographic protections are required? | Cache threat model and format design |
| OQ-09-09 | **[OPEN]** What fallback models/modes may serve after node loss? | Capacity matrix and client policy |
| OQ-09-10 | **[OPEN]** What security boundary applies to LAN users and administrators? | Threat model and identity design |
| OQ-09-11 | **[OPEN]** How are clock accuracy and cross-node timestamps validated? | Time-sync measurement plan |
| OQ-09-12 | **[OPEN]** Which results block release versus consume error budget? | Ratified severity policy |

## New gaps discovered

- Current sources expose server timings but product SLOs need client-visible timing and synchronized fault events.
- No acceptance corpus, requirement owner, waiver mechanism, or stable model/profile manifest exists yet.

