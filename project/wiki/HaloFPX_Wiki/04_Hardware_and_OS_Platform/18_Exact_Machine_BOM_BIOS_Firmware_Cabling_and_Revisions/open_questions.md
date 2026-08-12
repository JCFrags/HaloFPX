---
section_id: "18"
title: "BOM open questions"
status: "needs-machine-validation"
last_verified: "2026-07-16"
applies_to:
  repositories: []
  software_versions: []
  hardware_revisions: ["Nimo Direct MME3L / NIMO Mini PC (revision unknown)"]
related_sections: ["20", "21", "22", "23", "84"]
---

# Open questions

| ID | Question / dependency | Resolution |
|---|---|---|
| S18-OQ01 | What are both baseboard and chassis revisions? | S18-E01 plus label photos |
| S18-OQ02 | What exact PSU and cooling assemblies are installed? | S18-E02; link Section 22 |
| S18-OQ03 | What are cable manufacturer, SKU, length, certification, E-marker identity, and end orientation? | S18-E02 and USB-IF product lookup |
| S18-OQ04 | Which physical ports map to each USB4 domain/NHI/root/retimer chain? | S18-E02/E04; link Section 20 |
| S18-OQ05 | Are BIOS, EC, USB4 host-router/retimer, NVMe, CPU microcode, and amdgpu firmware byte-equivalent? | S18-E03 |
| S18-OQ06 | What BIOS settings are active for VRAM, IOMMU, ASPM, power, memory, fan, Secure Boot, and USB4 security? | Export/screenshots plus OS corroboration |
| S18-OQ07 | Is the July 12 hardware inventory still current? | Fresh same-window S18-E01 |
| S18-OQ08 | What mismatch threshold blocks distributed benchmarks? | Project decision after Sections 19–23 experiments |

No current open question is promoted to a project requirement without a decision record.
