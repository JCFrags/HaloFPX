---
section_id: "40"
title: "Replication Open Questions"
status: "needs-machine-validation"
last_verified: "2026-07-16"
applies_to:
  repositories: ["HaloFPX design candidate"]
  software_versions: []
  hardware_revisions: []
related_sections: ["38", "46", "48", "55"]
---

# Open questions

| ID | **[OPEN]** question | Needed evidence |
|---|---|---|
| DR40-O1 | What queue/cache score best predicts p99 completion on each workload? | `DR-40-E1` |
| DR40-O2 | What affinity guard avoids both cache loss and head-of-line delay? | skewed-load sweep |
| DR40-O3 | Can the chosen cache/checkpoint format move across nodes and commits? | compatibility/fault tests |
| DR40-O4 | What exact sampler/RNG state is needed for continuation? | source audit and replay test |
| DR40-O5 | Which models should be duplicated versus diversified? | product catalog plus reload results |
| DR40-O6 | What does the client observe on mid-stream node loss? | API/recovery decision |
| DR40-O7 | How are session/prefix routing keys privacy-protected and expired? | security/privacy review |
