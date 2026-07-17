---
section_id: "47"
title: "Planner Open Questions"
status: "needs-machine-validation"
last_verified: "2026-07-16"
applies_to:
  repositories: ["HaloFPX proposed runtime"]
  software_versions: []
  hardware_revisions: ["two matched AMD Strix Halo systems (exact BOM open)"]
related_sections: ["18", "20", "23", "37", "55", "67", "73"]
---

# Open questions

| ID | Question | Closure evidence |
|---|---|---|
| S47-OQ-01 | **[OPEN]** Which exact HIP/ROCm and Vulkan/Mesa stacks support every target op correctly on gfx1151? | Version-pinned backend correctness matrix. |
| S47-OQ-02 | **[OPEN]** Are both USB4 links independent, stable, and beneficial for which message sizes? | Topology inspection and dual-link curves. |
| S47-OQ-03 | **[OPEN]** What objective presets and hard SLOs does the product expose? | Section 09/product decision. |
| S47-OQ-04 | **[OPEN]** What statistical confidence and improvement margin justify promotion? | Benchmark policy and false-promotion analysis. |
| S47-OQ-05 | **[OPEN]** Which full model/quant/context combinations fit a single node? | Reproducible memory-capacity matrix. |
| S47-OQ-06 | **[OPEN]** Does backend choice alter acceptable logits/quality or cache compatibility? | Matched token/logit/quality and restore tests. |
| S47-OQ-07 | **[OPEN]** Can power profiles be selected without privileged runtime mutation? | OS/platform inspection and safe operating policy. |
| S47-OQ-08 | **[OPEN]** How long may a plan remain valid under workload drift? | Drift study and operational freshness policy. |

## Newly identified gaps

**[OPEN]** Define a canonical machine/build fingerprint schema shared by plans, caches, experiments, and fault reports.

**[OPEN]** Define how autotuning is resource-isolated so it cannot evict production cache state or violate user SLOs.
