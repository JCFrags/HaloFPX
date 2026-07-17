---
section_id: "20"
title: "USB4 topology open questions"
status: "needs-machine-validation"
last_verified: "2026-07-16"
applies_to:
  repositories: []
  software_versions: ["Linux USB4/Thunderbolt", "thunderbolt-net"]
  hardware_revisions: ["Nimo Direct MME3L / NIMO Mini PC (revision unknown)"]
related_sections: ["18", "19", "23", "50", "52", "55", "84"]
---

# Open questions

| ID | Question | Resolution |
|---|---|---|
| S20-OQ01 | Which labeled physical port maps to each domain/NHI/root on each node? | S20-E01/E02/E05 |
| S20-OQ02 | Are the two NHI functions on distinct PCI roots or only distinct functions behind shared fabric? | S20-E01 PCI/ACPI topology |
| S20-OQ03 | What are controller and retimer silicon revisions/NVM versions? | S20-E01 plus Section 18 firmware inventory |
| S20-OQ04 | What are exact cable SKUs, lengths, certification and E-marker identities? | S18-E02/S20-E02 |
| S20-OQ05 | Do simultaneous loads retain per-rail throughput and latency in both directions? | S20-E03 |
| S20-OQ06 | Is any deficit physical, memory-bandwidth, IRQ/CPU, thermal/power, or protocol limited? | S20-E03 controlled factors |
| S20-OQ07 | Does one cable pull/reset disturb the other domain or GPU? | S20-E04 |
| S20-OQ08 | Are domain/netdev identities stable across cold boot and attach order? | S20-E05 |
| S20-OQ09 | What security level and IOMMU DMA-protection policy is acceptable? | S19-E06 plus security decision |
| S20-OQ10 | Which dual-link policy wins for real HaloFPX messages? | Sections 52, 55 and 75 after independence gate |

**[OPEN]** None of these questions is resolved solely by the historical MPTCP validation.
