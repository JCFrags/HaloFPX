---
section_id: "64"
title: "Lifecycle and privacy open questions"
status: "needs-machine-validation"
last_verified: "2026-07-16"
applies_to:
  repositories: []
  software_versions: []
  hardware_revisions: []
related_sections: ["60", "65", "71"]
---

# Open questions

| ID | Question | Evidence needed |
|---|---|---|
| O64-01 | What are global/model/user byte and inode budgets? | capacity/SLO decision |
| O64-02 | How are shared physical bytes charged? | accounting ADR |
| O64-03 | Who may pin prefixes and for how long? | authorization policy |
| O64-04 | What deletion promise is user-facing? | privacy/retention policy |
| O64-05 | Is encryption per user, model, or filesystem? | threat model |
| O64-06 | How are backups and exports included in deletion? | operations design |
| O64-07 | What GC throttle protects p99 inference latency? | M64-04 |
| O64-08 | How much emergency reserve is required? | M64-02 |
| O64-09 | Are anonymous requests allowed to share content-derived state? | product/privacy decision |

