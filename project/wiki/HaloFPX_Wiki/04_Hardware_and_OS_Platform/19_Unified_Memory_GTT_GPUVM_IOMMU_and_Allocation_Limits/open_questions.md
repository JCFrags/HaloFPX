---
section_id: "19"
title: "Unified-memory open questions"
status: "needs-machine-validation"
last_verified: "2026-07-16"
applies_to:
  repositories: []
  software_versions: ["ROCm/HIP", "Linux amdgpu/TTM"]
  hardware_revisions: ["Radeon 8060S / gfx1151"]
related_sections: ["23", "24", "54", "74", "84"]
---

# Open questions

| ID | Question | Resolution |
|---|---|---|
| S19-OQ01 | What are current BIOS carve-out and effective TTM/GTT/VRAM values on both nodes? | S19-E01 plus BIOS record |
| S19-OQ02 | Which HIP/HMM/migration/coherence capabilities actually report supported? | S19-E01/E04 |
| S19-OQ03 | What safe per-rank allocation ceiling remains after OS, KV, workspace, cache, and transport reserves? | S19-E02/E05 |
| S19-OQ04 | Does two-process allocation change the ceiling or trigger the system-memory reservation path? | S19-E02 |
| S19-OQ05 | Which allocator/coherence policy is best for weights, KV, and transport staging? | S19-E03/E05; Section 54 |
| S19-OQ06 | What are the cost and fault behavior of managed migration on this exact stack? | S19-E04 |
| S19-OQ07 | What is the security, NPU, stability, and performance effect of IOMMU on versus off? | S19-E06; Section 20 |
| S19-OQ08 | Is the deprecated `amdgpu.gttsize` still necessary on the chosen kernel/ROCm pair? | Section 23 compatibility test |
| S19-OQ09 | What PSI/swap/fault thresholds predict unsafe model admission? | S19-E05 and Section 74 |

All settings remain contingent; none is promoted from the historical cluster audit without a current experiment.
