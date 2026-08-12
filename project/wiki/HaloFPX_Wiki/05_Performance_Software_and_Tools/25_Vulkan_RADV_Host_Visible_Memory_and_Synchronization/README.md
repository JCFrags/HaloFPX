---
section_id: "25"
title: "Vulkan, RADV, Host-Visible Memory, and Synchronization"
status: "needs-machine-validation"
last_verified: "2026-07-16"
applies_to:
  repositories:
    - "Mesa 20f4f9f45057559475600b60364b60643011990f"
    - "ggml-org/llama.cpp 788e07dc91d266ad3162a1ce9037665656269689"
  software_versions:
    - "Vulkan 1.4.357"
  hardware_revisions: []
related_sections: ["19", "23", "24", "37", "47", "54", "74", "75"]
---

# Vulkan, RADV, Host-Visible Memory, and Synchronization

## Decision-oriented summary

- **[VERIFIED]** Vulkan exposes memory heaps and memory types separately. `HOST_VISIBLE` permits mapping, `HOST_COHERENT` removes the need for explicit mapped-range flush/invalidate operations, and `HOST_CACHED` describes host caching. Coherence does **not** replace execution or device-memory dependencies. [S25-01]
- **[VERIFIED]** Mesa RADV main at `20f4f9f45057559475600b60364b60643011990f` recognizes `CHIP_STRIX_HALO` as `gfx1151`. Its physical-device code can expose device-local/host-visible/coherent memory and host-visible/coherent/cached GTT memory, but the actual heaps, type indices, budgets, and performance are runtime-dependent. [S25-03]
- **[VERIFIED]** That RADV snapshot advertises buffer device address, synchronization2, descriptor buffer, pipeline binary, and subgroup-size control. Timeline semaphores are conditional on kernel timeline-syncobj support; external host memory is conditional on userptr support. [S25-03]
- **[INFERENCE]** Vulkan external-memory file descriptors are same-host interoperability mechanisms, not a remote-memory transport across the two USB4-connected machines. HaloFPX still needs an explicit transport protocol and per-rank ownership transition. [S25-01] [S25-02]
- **[VERIFIED]** llama.cpp commit `788e07dc91d266ad3162a1ce9037665656269689` uses timeline semaphores for asynchronous transfer/compute coordination, buffer device address when supported, reusable command-buffer pools, and per-dispatch descriptor-set updates. Its compute pipelines are created with `VK_NULL_HANDLE` rather than an application-managed `VkPipelineCache`. [S25-06]
- **[RECOMMENDATION]** Keep Vulkan/RADV as a measured backend candidate, not the default distributed token-path backend. Compare it with HIP using matched models, quantizations, command/graph reuse, synchronization boundaries, and transport-buffer paths before section 47 selects a backend.

No Strix Halo measurements were performed for this section. Every platform-specific conclusion remains contingent on the experiments in [procedures_and_checks.md](procedures_and_checks.md).

## Page map

- [facts_and_constraints.md](facts_and_constraints.md) — sourced API, RADV, and llama.cpp facts.
- [design_implications.md](design_implications.md) — implications and HIP comparison.
- [procedures_and_checks.md](procedures_and_checks.md) — reproducible Internet and machine checks.
- [open_questions.md](open_questions.md) — unresolved choices and decision gates.
- [sources.md](sources.md) — primary-source register.

## Research split

1. **Internet/source research completed:** Vulkan 1.4.357 semantics, current RADV source and documentation, current llama.cpp Vulkan implementation, and HIP runtime documentation.
2. **Machine work required:** enumerate both machines, validate extension/handle properties, measure mapped-memory paths, submission/descriptor/pipeline costs, run correctness litmus tests, and compare Vulkan with HIP.
3. **Contingent decisions:** backend selection, direct host-visible transport buffers, timeline use, descriptor strategy, command reuse, and pipeline-cache policy.

## Applicability boundary

**[ASSUMPTION]** The target is two matched Linux Strix Halo systems using RADV and `amdgpu`. Driver packages, kernel/firmware versions, BIOS UMA allocation, and hardware revisions have not been captured here. Section 23 must freeze those versions before a measured result can be generalized even to the two project machines.

