---
section_id: "67"
title: "Configuration and Manifest Open Questions"
status: "needs-machine-validation"
last_verified: "2026-07-16"
applies_to:
  repositories: ["HaloFPX project"]
  software_versions: []
  hardware_revisions: []
related_sections: ["18", "23", "29", "38", "49", "60", "66", "68", "71", "72"]
---

# Open questions

| ID | Question | Needed evidence/decision |
|---|---|---|
| OQ-67-01 | **[OPEN]** Is YAML or JSON the human-authored canonical format? | Tooling/operations ADR |
| OQ-67-02 | **[OPEN]** Which environment/CLI fields may override files in production? | Security and operations review |
| OQ-67-03 | **[OPEN]** Which exact fields enter each compatibility hash? | Cache/wire/state design |
| OQ-67-04 | **[OPEN]** How are manifests signed and publishers authorized? | Threat model and key management |
| OQ-67-05 | **[OPEN]** Which secret provider/reference syntax is supported? | Section 71 decision |
| OQ-67-06 | **[OPEN]** What exact hardware identifiers define a compatible class versus one node? | Two-node inventory and reproducibility tests |
| OQ-67-07 | **[OPEN]** How are volatile measurements expired and refreshed? | Profile lifecycle policy |
| OQ-67-08 | **[OPEN]** Which model/shard metadata is authoritative when filenames disagree with GGUF? | Model-admission ADR |
| OQ-67-09 | **[OPEN]** How many schema generations must upgrade/rollback support? | Release policy |
| OQ-67-10 | **[OPEN]** Can request hints choose an unbenchmarked candidate plan? | Product and safety policy |
| OQ-67-11 | **[OPEN]** How are node-specific absolute paths mapped without entering semantic hashes? | Deployment layout design |

## New gaps discovered

- No exact hardware profile, model manifest, plan schema, signing authority, or canonical hash field set exists.
- Upstream exposes overlapping CLI/environment controls; accepting all of them would undermine explainable precedence.

