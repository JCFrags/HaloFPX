---
section_id: "23"
title: "Software compatibility open questions"
status: "needs-machine-validation"
last_verified: "2026-07-16"
applies_to:
  repositories: ["HaloFPX", "ROCmFPX"]
  software_versions: []
  hardware_revisions: ["two project Strix Halo nodes"]
related_sections: ["13", "18", "19", "20", "37", "50", "70", "81"]
---

# Software compatibility open questions

| ID | Open question | Closure evidence |
|---|---|---|
| 23-OQ01 | **[OPEN]** What exact distribution, kernel source/config, amdgpu, and boot parameters run on each node? | Paired immutable inventory |
| 23-OQ02 | **[OPEN]** Do both running kernels contain both required KFD fixes? | ancestry or distribution backport evidence plus smoke |
| 23-OQ03 | **[OPEN]** Which firmware files are loaded, from what package/revision, with what hashes? | boot log, package manifest, SHA-256 |
| 23-OQ04 | **[OPEN]** Which ROCm/HIP/HSA packages and compiler versions are installed and do they enumerate/target `gfx1151` correctly? | package and smoke-test evidence |
| 23-OQ05 | **[OPEN]** Does the intended ROCmFPX commit build and run correctly on the supported 7.2.1 tuple and newer 7.14.0 lane? | pinned builds and correctness tests |
| 23-OQ06 | **[OPEN]** What Mesa/RADV version, Vulkan API, and required extensions are actually exposed? | `vulkaninfo` JSON and loaded-library trace |
| 23-OQ07 | **[OPEN]** Which current Strix Halo defects affect the selected kernels/ROCm/Mesa and workload? | official issue/release-note mapping plus reproducer |
| 23-OQ08 | **[OPEN]** Is Linux 7.2 stable/final in the chosen distribution and is `thunderbolt-stream` enabled? | release/package/config evidence |
| 23-OQ09 | **[OPEN]** Can USB4STREAM coexist with correct HIP/Vulkan inference on each link and both links? | transport + compute + error traces |
| 23-OQ10 | **[OPEN]** Is a backport maintainable, or should production wait for a supported distribution kernel? | patch inventory, CI matrix, measured benefit/risk |
| 23-OQ11 | **[OPEN]** What rollback kernel, packages, firmware manifest, and single-node fallback are verified? | recovery drill |
| 23-OQ12 | **[OPEN]** Does AMD's noted ROCm 7.2.1 LLM performance issue reproduce in the target models/backends? | matched ROCm-version benchmark with correctness |

Production selection is blocked on OQ01-OQ06 and OQ11. USB4STREAM promotion is additionally blocked on OQ08-OQ10.
