---
section_id: "10"
title: "Architecture Principle Open Questions"
status: "needs-machine-validation"
last_verified: "2026-07-16"
applies_to:
  repositories: ["HaloFPX project"]
  software_versions: []
  hardware_revisions: []
related_sections: ["06", "07", "09", "38", "47", "48", "49"]
---

# Open questions

| ID | Question | Needed evidence/decision |
|---|---|---|
| OQ-10-01 | **[OPEN]** What are the ratified decision weights by workload? | Sponsor and workload review |
| OQ-10-02 | **[OPEN]** What quality/correctness tolerances permit backend/cache differences? | Model-specific evaluation ADR |
| OQ-10-03 | **[OPEN]** Is capacity-only distribution a release success when latency is worse? | Charter decision |
| OQ-10-04 | **[OPEN]** What maximum upstream patch delta and rebase burden are acceptable? | Maintenance budget |
| OQ-10-05 | **[OPEN]** Which token-path operations are allowed in coordinator versus ranks? | Protocol profiling/design |
| OQ-10-06 | **[OPEN]** When may state migrate between ranks, if ever? | State semantics and recovery proof |
| OQ-10-07 | **[OPEN]** What automatic fallback changes require client consent? | API/product policy |
| OQ-10-08 | **[OPEN]** How long may a tuning profile remain valid without remeasurement? | Volatility data and review policy |
| OQ-10-09 | **[OPEN]** Are both USB4 links independent enough for striping, or should one hedge/fail over? | Section 49–55 measurements |

## New gaps discovered

- No ratified tradeoff weights, architecture-decision authority, or profile-expiry policy exists.
- “Upstreamable” needs a concrete patch-budget and contribution strategy, not only a preference.

