---
section_id: "07"
title: "Users and Workloads Open Questions"
status: "needs-machine-validation"
last_verified: "2026-07-16"
applies_to:
  repositories: ["HaloFPX project"]
  software_versions: []
  hardware_revisions: []
related_sections: ["06", "08", "09", "46", "69"]
---

# Open questions

| ID | Question | Needed evidence |
|---|---|---|
| OQ-07-01 | **[OPEN]** Which persona is primary at initial release? | Sponsor-ranked persona list |
| OQ-07-02 | **[OPEN]** What are prompt, cached-prefix, and output token percentiles? | Privacy-reviewed trace |
| OQ-07-03 | **[OPEN]** How many turns and how long must conversations persist? | Usage sample and retention policy |
| OQ-07-04 | **[OPEN]** What concurrency and burst levels must be supported? | Client inventory and trace |
| OQ-07-05 | **[OPEN]** Which coding languages, tools, and schemas are acceptance-critical? | Versioned task suite |
| OQ-07-06 | **[OPEN]** Which models/quantizations must coexist or swap? | Model catalog and storage plan |
| OQ-07-07 | **[OPEN]** Is authenticated LAN multi-user service in v1? | Scope and threat-model decision |
| OQ-07-08 | **[OPEN]** What data may be logged, cached, backed up, or exported? | Privacy/retention policy |
| OQ-07-09 | **[OPEN]** How should interactive traffic preempt or coexist with batch work? | Fairness policy and mixed-load test |
| OQ-07-10 | **[OPEN]** Which offline functions must work without DNS, time sync, or package registries? | Offline acceptance scenario |

## New gaps discovered

- No authoritative workload corpus, client inventory, privacy classification, or workload prioritization exists yet.
- Cache value depends on canonical prompt serialization; client/template stability needs ownership and versioning.

