---
section_id: "63"
title: "Durability open questions"
status: "needs-machine-validation"
last_verified: "2026-07-17"
applies_to:
  repositories: []
  software_versions: []
  hardware_revisions: []
related_sections: ["21", "58", "63"]
---

# Open questions

| ID | Question | Evidence needed |
|---|---|---|
| O63-01 | Which filesystem, mount options, SSDs and firmware are authoritative? | machine inventory |
| O63-02 | What exact failure model does each mode promise? | product decision |
| O63-03 | Which strong digest and manifest encoding are selected? | ADR and benchmarks |
| O63-04 | How many valid generations are retained? | recovery/space policy |
| O63-05 | Who commits during coordinator failure/failover? | section 58 protocol |
| O63-06 | Can ranks acknowledge prepared state without durable network consensus? | failure analysis |
| O63-07 | What quarantine retention and operator workflow apply? | section 65 tooling |
| O63-08 | Do device volatile caches honor the tested flush sequence? | M63-03 power tests |
| O63-09 | **[RESOLVED 2026-07-18]** P63-00 uses split exhaustive TLC safety/chain/two-lineage/liveness configurations, explicit weak fairness only in the reduced liveness model, five required negative variants, and an Apalache type/bounded-safety cross-check. | P63-00 final evidence and independent adversarial review [S63-07] |
