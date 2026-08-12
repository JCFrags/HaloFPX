---
section_id: "72"
title: "Upgrade, Migration, and Recovery Open Questions"
status: "needs-machine-validation"
last_verified: "2026-07-16"
applies_to:
  repositories: ["Custom_Inference_Project"]
  software_versions: ["operations design candidate 2026-07-16"]
  hardware_revisions: ["dual Strix Halo target; exact nodes open"]
related_sections: ["67", "68", "70", "71"]
---

# Open questions

1. **[OPEN]** What API, peer-protocol, config, plan, model, and cache compatibility window is promised?
2. **[OPEN]** Which modes/releases are eligible for rolling upgrade, and who approves the matrix?
3. **[OPEN]** What drain deadline and cancellation/resume semantics apply to streaming requests?
4. **[OPEN]** What are the ratified RPO and RTO for configuration, user state, audit data, models, and cache?
5. **[OPEN]** Where are encrypted backups stored, who controls recovery keys, and how is restore authorization audited?
6. **[OPEN]** How many releases/config snapshots/backups/support bundles are retained and for how long?
7. **[OPEN]** Will any legacy cache format be migrated, or always invalidated and recomputed?
8. **[OPEN]** Which durable session/user state, if any, is a product guarantee?
9. **[OPEN]** What constitutes a successful post-upgrade semantic canary before traffic resumes?
10. **[OPEN]** Who has authority to initiate rollback, destructive cache purge, credential rotation, and disaster recovery?
