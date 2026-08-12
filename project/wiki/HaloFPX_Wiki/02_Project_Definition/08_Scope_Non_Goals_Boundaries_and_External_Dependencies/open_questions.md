---
section_id: "08"
title: "Scope and Boundary Open Questions"
status: "needs-machine-validation"
last_verified: "2026-07-16"
applies_to:
  repositories: ["HaloFPX project"]
  software_versions: []
  hardware_revisions: []
related_sections: ["06", "07", "09", "11", "18", "23", "49", "69"]
---

# Open questions

| ID | Question | Owner/evidence needed |
|---|---|---|
| OQ-08-01 | **[OPEN]** What exact machines, firmware, cables, ports, and NVMe revisions define supported hardware? | Section 18 inventory |
| OQ-08-02 | **[OPEN]** Which Linux/kernel/ROCm/Mesa versions are supported? | Section 23 matrix and boot tests |
| OQ-08-03 | **[OPEN]** Which llama.cpp ancestor is the integration base? | Lineage analysis and ADR |
| OQ-08-04 | **[OPEN]** Which CachyLLama changes are imported versus reimplemented? | Source audit and license/patch review |
| OQ-08-05 | **[OPEN]** Which API endpoints and client libraries are contractual? | Client inventory and conformance suite |
| OQ-08-06 | **[OPEN]** Is LAN multi-user exposure a v1 requirement? | Scope/threat-model decision |
| OQ-08-07 | **[OPEN]** Which models, contexts, and quantizations are supported? | Model/quality/capacity matrix |
| OQ-08-08 | **[OPEN]** Are the two USB4 paths independent and safely aggregateable? | Topology and fault measurements |
| OQ-08-09 | **[OPEN]** What cache data is sensitive and how is it encrypted/backed up/erased? | Data classification and security design |
| OQ-08-10 | **[OPEN]** Which optional execution modes must ship in v1? | Workload evidence and architecture decisions |

## New gaps discovered

- There is no frozen dependency graph or known common ancestor among the intended forks.
- API compatibility, cache-format compatibility, and the dual-link transport are distinct contracts and need separate versioning.

