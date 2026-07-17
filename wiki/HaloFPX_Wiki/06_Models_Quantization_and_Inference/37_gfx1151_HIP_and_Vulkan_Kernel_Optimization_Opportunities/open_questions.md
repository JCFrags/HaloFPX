---
section_id: "37"
title: "gfx1151 optimization open questions"
status: "needs-machine-validation"
last_verified: "2026-07-16"
applies_to:
  repositories: []
  software_versions: []
  hardware_revisions: ["gfx1151"]
related_sections: ["24", "25", "27", "28", "74"]
---

# Open questions

| ID | Question | Evidence needed |
|---|---|---|
| O37-01 | What are the top wall-time kernels for each required workload? | M37-01 traces |
| O37-02 | Which exact ROCm/LLVM, driver, and Vulkan stack is the project baseline? | section 24/25 pinned matrix |
| O37-03 | Which wave/subgroup sizes and operations are exposed on each machine? | device query and compiled ISA |
| O37-04 | Are current ROCmFPX defaults optimal for both matched machines? | M37-02 repeated sweep |
| O37-05 | Which graph shapes are reusable under continuous batching? | scheduler trace and correctness tests |
| O37-06 | Where do dequant/GEMV fusion gains lose to register pressure? | counters plus shape sweep |
| O37-07 | Can row-parallel reductions overlap the dual links? | section 42/52 timeline experiment |
| O37-08 | Which Vulkan shader variants compile differently under RADV versus AMDVLK? | exact-driver shader/ISA comparison |
| O37-09 | What numerical tolerance applies per fused operation/quant? | section 78 policy |
| O37-10 | Which ROCmFPX experiment results reproduce on this hardware? | clean rerun with preserved logs |

