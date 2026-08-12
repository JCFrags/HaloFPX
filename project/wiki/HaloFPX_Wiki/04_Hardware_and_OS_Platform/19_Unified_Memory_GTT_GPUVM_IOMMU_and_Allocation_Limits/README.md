---
section_id: "19"
title: "Unified Memory, GTT, GPUVM, IOMMU, and Allocation Limits"
status: "needs-machine-validation"
last_verified: "2026-07-17"
applies_to:
  repositories: []
  software_versions: ["ROCm 7.14 documentation", "Linux amdgpu/TTM"]
  hardware_revisions: ["Radeon 8060S / gfx1151"]
related_sections: ["17", "18", "23", "24", "54", "74", "84"]
---

# 19 — Unified Memory, GTT, GPUVM, IOMMU, and Allocation Limits

Strix Halo CPU and GPU share physical LPDDR5X, but firmware carve-out, TTM/GTT accounting, GPUVM mappings, HIP allocation semantics, and system-memory policy still impose distinct limits. “128 GB unified memory” is not an allocatable-model guarantee.

## High-value conclusions

- **[VERIFIED]** AMD documents Radeon 8060S/gfx1151 as an RDNA 3.5 APU with dynamic plus carve-out memory [S19-01].
- **[VERIFIED]** GPUVM gives per-process GPU address spaces whose page tables can map VRAM and system pages; TTM manages buffer-object placement, movement, lifetime, and CPU mappings [S19-02, S19-03].
- **[VERIFIED]** AMD recommends a small BIOS VRAM reservation and increasing TTM shared-memory limits for gfx1151 when capacity is needed; limits are in pages, not bytes [S19-04].
- **[VERIFIED]** Preserved July 12 audit reports record 1 GiB firmware VRAM, 126,976 MiB GTT, `ttm.pages_limit=32505856`, `amdgpu.no_system_mem_limit=Y`, and `amd_iommu=off` on the cluster [S19-L01, S19-L02]. This is historical report evidence, not a safe universal configuration; because the raw command-output bundle was unavailable, S19-E01 remains the reproducible measurement gate.
- **[MEASURED]** The 2026-07-17 live snapshot confirmed about 124 GiB host memory per node, 133,143,986,176 bytes of reported GTT capacity, the 126,976 MiB GTT and 32,505,856-page TTM command-line settings, `amd_iommu=off`, and a 32 GiB swapfile on both nodes [S19-L03]. Swap policy differed between hosts.
- **[OPEN]** Allocator capabilities, fault/migration behavior, bandwidth, contention, fragmentation, and safe reserve still require S19-E02–E06; the inventory alone does not establish a 200–230 GB model budget.

See [facts](facts_and_constraints.md), [implications](design_implications.md), [procedures](procedures_and_checks.md), [questions](open_questions.md), and [sources](sources.md).
