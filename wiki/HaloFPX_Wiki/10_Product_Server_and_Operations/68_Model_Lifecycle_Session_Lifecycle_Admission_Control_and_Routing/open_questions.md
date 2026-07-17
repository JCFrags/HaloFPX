---
section_id: "68"
title: "Lifecycle and Routing Open Questions"
status: "needs-machine-validation"
last_verified: "2026-07-16"
applies_to:
  repositories: ["HaloFPX project"]
  software_versions: []
  hardware_revisions: []
related_sections: ["07", "09", "38", "46", "48", "60", "61", "66", "67", "69", "72"]
---

# Open questions

| ID | Question | Needed evidence/decision |
|---|---|---|
| OQ-68-01 | **[OPEN]** Which models/plans preload at cold boot and which load on demand? | Workload trace and load measurements |
| OQ-68-02 | **[OPEN]** What memory headroom and reservation formula apply per backend/mode? | Peak/steady measurements |
| OQ-68-03 | **[OPEN]** Are sessions server-authoritative or reconstructed from each client prompt? | API/product decision |
| OQ-68-04 | **[OPEN]** What session idle/retention/expiry periods and deletion semantics apply? | Privacy/storage policy |
| OQ-68-05 | **[OPEN]** Can committed session state migrate between replicas, and in what format? | State compatibility/fault proof |
| OQ-68-06 | **[OPEN]** What per-user/global/model queue and concurrency caps are required? | Workload trace and fairness policy |
| OQ-68-07 | **[OPEN]** What weights and reserved capacity separate interactive from batch traffic? | Mixed-load experiments |
| OQ-68-08 | **[OPEN]** Which one-link/single-node fallback plans are pre-admitted per model? | Capacity and fault matrix |
| OQ-68-09 | **[OPEN]** What is the committed boundary for retry after partial prompt/decode work? | Rank protocol and API ADR |
| OQ-68-10 | **[OPEN]** How is coordinator metadata made durable without accepting stale epochs? | Recovery design and crash tests |
| OQ-68-11 | **[OPEN]** How are model replacement and cache invalidation coordinated across ranks? | Generation/compatibility design |
| OQ-68-12 | **[OPEN]** Is upstream sleep-on-idle compatible with required availability and persistent sessions? | Startup/SLO and restore tests |

## New gaps discovered

- No authoritative session model, ownership/epoch protocol, reservation formula, fairness policy, or pre-admitted fallback table exists.
- Upstream multi-model routing is useful evidence but does not define atomic two-rank load/readiness or distributed replacement.

