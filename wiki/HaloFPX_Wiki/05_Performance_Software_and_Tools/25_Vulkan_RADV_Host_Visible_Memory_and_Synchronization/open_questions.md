---
section_id: "25"
title: "Vulkan and RADV Open Questions"
status: "needs-machine-validation"
last_verified: "2026-07-16"
applies_to:
  repositories:
    - "Mesa 20f4f9f45057559475600b60364b60643011990f"
    - "ggml-org/llama.cpp 788e07dc91d266ad3162a1ce9037665656269689"
  software_versions:
    - "Vulkan 1.4.357"
  hardware_revisions: []
related_sections: ["19", "23", "24", "37", "47", "54", "74"]
---

# Open questions

| ID | Question | Evidence needed | Blocks |
|---|---|---|---|
| O25-01 | **[OPEN]** What heaps, types, budgets, and type indices do both frozen Strix Halo machines actually expose? | R25-M1 on both ranks with section 23 version inventory. | Allocation policy. |
| O25-02 | **[OPEN]** Does device-local+host-visible memory provide a lower end-to-end GPU-to-USB4 path than explicit staging under concurrent inference? | R25-M3 and R25-M6, including socket/network transfer. | Section 54 zero-copy choice. |
| O25-03 | **[OPEN]** Which external memory/semaphore FD handle combinations are importable/exportable, and can any required timeline payload be shared with the chosen same-host component? | Handle-specific property queries and R25-M5. | Same-rank API interop. |
| O25-04 | **[OPEN]** Is `has_timeline_syncobj` true with the frozen kernel/amdgpu stack on both machines? | R25-M1 plus kernel/Mesa source-version record. | Timeline-based slot protocol. |
| O25-05 | **[OPEN]** What subgroup size is correct and fastest per inference kernel on gfx1151? | R25-M9 plus section 37 shader variants. | Kernel tuning. |
| O25-06 | **[OPEN]** Is per-dispatch descriptor update a material CPU/tail-latency cost in the frozen llama.cpp backend? | R25-M7 CPU profiling. | Descriptor-buffer work. |
| O25-07 | **[OPEN]** Which graph regions have stable enough resources and dimensions for legal command-buffer reuse? | Graph-shape audit plus R25-M4/M7. | Persistent command strategy. |
| O25-08 | **[OPEN]** Does an application-managed `VkPipelineCache` improve startup beyond RADV's Mesa disk cache for the frozen shader set? | R25-M8 cold/warm matrix. | Cache implementation. |
| O25-09 | **[OPEN]** What is the invalidation fingerprint for shader/pipeline artifacts across Mesa, LLVM/ACO, kernel, firmware, shader generator, and specialization changes? | Controlled upgrade experiments and cache headers. | Safe cache reuse/rollback. |
| O25-10 | **[OPEN]** Does Vulkan meet correctness and matched performance thresholds versus HIP for each workload class? | R25-M10 and section 74 workload matrix. | Section 47 backend selection. |
| O25-11 | **[OPEN]** What timeout and cancellation protocol guarantees that a failed rank cannot cause early reuse or indefinite waits? | Fault injection across R25-M4 and section 48. | Distributed recovery. |
| O25-12 | **[OPEN]** Which exact llama.cpp/ROCmFPX commit becomes the Vulkan implementation baseline? | Sections 11–15 lineage decisions. | Reproducibility and source re-audit. |

## Research follow-up triggers

- **[RECOMMENDATION]** Recheck Mesa RADV source when the frozen Mesa release changes or a Strix Halo-specific Vulkan fix lands.
- **[RECOMMENDATION]** Recheck Vulkan backend internals whenever llama.cpp changes descriptor allocation, command submission, shader generation, host-visible memory, pipeline creation, or async transfer code.
- **[RECOMMENDATION]** Reopen the HIP comparison when ROCmFPX changes its graph, memory, collective, or gfx1151 kernel paths.
- **[RECOMMENDATION]** Convert an open question to a measured conclusion only with linked raw evidence and complete environment metadata.

