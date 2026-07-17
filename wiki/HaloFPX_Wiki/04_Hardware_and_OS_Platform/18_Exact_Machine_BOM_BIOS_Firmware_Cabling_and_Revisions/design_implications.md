---
section_id: "18"
title: "BOM design implications"
status: "needs-machine-validation"
last_verified: "2026-07-16"
applies_to:
  repositories: []
  software_versions: []
  hardware_revisions: ["Nimo Direct MME3L / NIMO Mini PC (revision unknown)"]
related_sections: ["19", "20", "21", "22", "23", "39", "55"]
---

# Design implications

- **[RECOMMENDATION]** Make the inventory manifest part of every benchmark run ID. Refuse matched-pair comparisons when any hard-match field differs or is missing.
- **[INFERENCE]** Firmware or cable asymmetry can masquerade as a scheduler, IRQ, or transport defect. Section 20 independence tests must bind each result to physical port and cable identities, not only `thunderbolt0/1`, whose numbering can change.
- **[RECOMMENDATION]** Store a stable logical mapping (`node`, chassis label, port label, cable label, domain UUID hash, netdev MAC hash) separately from ephemeral Linux names.
- **[RECOMMENDATION]** Treat NVMe wear and cooling as covariates rather than hard equality. Section 21 and 22 should define admissible deltas.
- **[RECOMMENDATION]** A firmware normalization change requires its own before/after receipt and rerun of USB4 topology, memory-capacity, thermal, and backend smoke gates; never silently replace the known-good state.
- **[OPEN]** If board revisions differ, the acceptable path depends on whether root-port, retimer, memory, or power behavior differs under matched tests.

## Gate

HaloFPX may use replication while the pair is only functionally similar, but tensor/pipeline or dual-rail performance claims should remain blocked until S18-E01–E04 establish reproducible identity and enumeration.
