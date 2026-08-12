---
section_id: "17"
title: "Strix Halo open questions"
status: "needs-machine-validation"
last_verified: "2026-07-16"
applies_to:
  repositories: []
  software_versions: []
  hardware_revisions: ["Two project Strix Halo machines; exact revisions OPEN"]
related_sections: ["18", "19", "20", "22", "23", "24", "25", "37", "74"]
---

# Open questions

| ID | Question | Why it matters | Resolution evidence | Dependency |
|---|---|---|---|---|
| S17-OQ-001 | **[OPEN]** What CPU complex/CCD and shared-L3 topology does each machine expose? | CPU worker/IRQ placement and cross-cache traffic. | S17-EXP-001 raw topology on both hosts. | 18 |
| S17-OQ-002 | **[OPEN]** Do both hosts report Radeon 8060S as native `gfx1151` with 40 enabled CUs? | Prevents building a plan for a nominal rather than installed configuration. | S17-EXP-002 plus firmware/BOM record. | 18, 23 |
| S17-OQ-003 | **[OPEN]** What are physical RAM, firmware carveout, HSA pool, GTT/GPUVM, and Vulkan heap budgets on each host? | Sets safe weights/KV/workspace/transport limits. | S17-EXP-002/004 and section 19 allocation tests. | 19 |
| S17-OQ-004 | **[OPEN]** Which FP16, BF16, INT8, INT4 and WMMA paths compile and pass numerical tests with the pinned stack? | Fast paths cannot be enabled from architecture family names alone. | S17-EXP-003 with exact commits and raw output. | 23, 37 |
| S17-OQ-005 | **[OPEN]** Which Vulkan subgroup/cooperative-matrix features and formats does the installed RADV/driver combination expose correctly? | Determines Vulkan kernel design and fallback requirements. | S17-EXP-004 plus correctness workload. | 25, 37 |
| S17-OQ-006 | **[OPEN]** What effective bandwidth remains under CPU, GPU, and concurrent load? | Tests the shared-memory contention model and distributed staging cost. | S17-EXP-005 matched data. | 22, 73, 74 |
| S17-OQ-007 | **[OPEN]** Which OEM power modes are actually available and sustainable without throttling? | Selects serving concurrency and comparison controls. | Firmware inventory and S17-EXP-005 time series. | 18, 22 |
| S17-OQ-008 | **[OPEN]** Are the two exposed USB4 ports independently routed and simultaneously performant? | Required before dual-link striping or failover design. | Section 20 topology and simultaneous-load experiments. | 20 |

## Internet follow-up backlog

- **[RECOMMENDATION]** On each pinned ROCm/LLVM/Mesa upgrade, diff the `gfx1151` target mapping, compatibility matrix, rocWMMA support, and RADV feature gates before rebuilding.
- **[RECOMMENDATION]** Seek an AMD-published Strix Halo CPU/cache/interconnect technical document. Until one is available, retain S17-OQ-001 rather than promoting third-party die diagrams.
- **[RECOMMENDATION]** Track component-level ROCm support, not only the umbrella compatibility page; record library regressions or gaps against exact releases.

## Decision gates

- Backend selection waits for S17-OQ-004/005 plus section 74 matched benchmarks.
- Memory budgets wait for S17-OQ-003/006.
- CPU affinity waits for S17-OQ-001.
- Dual-link distributed modes wait for S17-OQ-008.
- Production power profile waits for S17-OQ-007.
