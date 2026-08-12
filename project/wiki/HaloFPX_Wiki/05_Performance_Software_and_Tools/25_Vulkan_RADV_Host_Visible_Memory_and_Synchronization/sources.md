---
section_id: "25"
title: "Vulkan and RADV Source Register"
status: "needs-machine-validation"
last_verified: "2026-07-16"
applies_to:
  repositories:
    - "Mesa 20f4f9f45057559475600b60364b60643011990f"
    - "ggml-org/llama.cpp 788e07dc91d266ad3162a1ce9037665656269689"
  software_versions:
    - "Vulkan 1.4.357"
  hardware_revisions: []
related_sections: ["02", "11", "23", "24", "47", "54"]
---

# Sources

Access date for every Internet source: **2026-07-16**.

## S25-01 — Vulkan specification

- **Publisher:** Khronos Group
- **Revision:** Vulkan 1.4.357; Vulkan-Docs commit `d184375dcc5da2b06ca375a8d7d1f9d21ca64a76`
- **URLs:** [Memory](https://docs.vulkan.org/spec/latest/chapters/memory.html), [Synchronization](https://docs.vulkan.org/spec/latest/chapters/synchronization.html), [Command buffers](https://docs.vulkan.org/spec/latest/chapters/cmdbuffers.html), [Pipelines](https://docs.vulkan.org/spec/latest/chapters/pipelines.html), [Descriptors](https://docs.vulkan.org/spec/latest/chapters/descriptorsets.html), [Capabilities](https://docs.vulkan.org/spec/latest/chapters/capabilities.html), [pinned registry XML](https://github.com/KhronosGroup/Vulkan-Docs/blob/d184375dcc5da2b06ca375a8d7d1f9d21ca64a76/xml/vk.xml)
- **Supports:** normative memory flags, flush/invalidate, dependencies, timeline semaphores, external handles, command lifecycle/submission, descriptor rules, pipeline cache compatibility, and subgroup semantics.
- **Limitations:** API guarantees only; does not establish RADV or Strix Halo performance.

## S25-02 — Vulkan Guide

- **Publisher:** Khronos Group
- **Revision:** Vulkan-Guide commit `fc39d8eb4782c14debfbf953905d79d46a228e66`
- **URLs:** [Memory allocation](https://docs.vulkan.org/guide/latest/memory_allocation.html), [Synchronization examples](https://docs.vulkan.org/guide/latest/synchronization_examples.html), [External memory and synchronization](https://docs.vulkan.org/guide/latest/extensions/external.html), [Pipeline cache](https://docs.vulkan.org/guide/latest/pipeline_cache.html), [Subgroups](https://docs.vulkan.org/guide/latest/subgroups.html)
- **Supports:** practical UMA/staging interpretation, synchronization examples, external-handle workflow, cache use, and subgroup-size guidance.
- **Limitations:** explanatory guidance and samples; sample performance is not HaloFPX evidence.

## S25-03 — Mesa RADV and AMD source

- **Repository:** Mesa
- **Commit/date:** `20f4f9f45057559475600b60364b60643011990f`, committed 2026-07-16
- **URLs:** [`radv_physical_device.c`](https://gitlab.freedesktop.org/mesa/mesa/-/blob/20f4f9f45057559475600b60364b60643011990f/src/amd/vulkan/radv_physical_device.c), [`radv_device.c`](https://gitlab.freedesktop.org/mesa/mesa/-/blob/20f4f9f45057559475600b60364b60643011990f/src/amd/vulkan/radv_device.c), [`amd_family.c`](https://gitlab.freedesktop.org/mesa/mesa/-/blob/20f4f9f45057559475600b60364b60643011990f/src/amd/common/amd_family.c), [`amd_family.h`](https://gitlab.freedesktop.org/mesa/mesa/-/blob/20f4f9f45057559475600b60364b60643011990f/src/amd/common/amd_family.h)
- **Supports:** Strix Halo/gfx1151 recognition, RADV heap/type construction, feature and extension gates, subgroup properties, and pipeline-cache UUID source.
- **Limitations:** moving main-branch snapshot, not a frozen release; runtime capability is conditional on kernel/device information.

## S25-04 — RADV documentation

- **Publisher:** Mesa Project
- **Revision/date:** documentation rendered from current Mesa documentation; accessed 2026-07-16
- **URL:** [RADV](https://docs.mesa3d.org/drivers/radv.html)
- **Supports:** RADV/amdgpu responsibility boundary and SPIR-V -> NIR -> ACO compilation path.
- **Limitations:** “latest” documentation is mutable; exact behavior is pinned through S25-03 instead.

## S25-05 — Mesa environment variables

- **Publisher:** Mesa Project
- **Revision/date:** current documentation; accessed 2026-07-16
- **URL:** [Environment variables](https://docs.mesa3d.org/envvars.html)
- **Supports:** Mesa shader-cache location, disable, size, and statistics controls.
- **Limitations:** variables and cache implementation may vary by Mesa build; not an application `VkPipelineCache` guarantee.

## S25-06 — llama.cpp Vulkan backend

- **Repository:** ggml-org/llama.cpp
- **Commit:** `788e07dc91d266ad3162a1ce9037665656269689`
- **URL:** [`ggml-vulkan.cpp`](https://github.com/ggml-org/llama.cpp/blob/788e07dc91d266ad3162a1ce9037665656269689/ggml/src/ggml-vulkan/ggml-vulkan.cpp)
- **Supports:** buffer-device-address detection, descriptor pool/set behavior, per-dispatch updates, command-buffer pooling, timeline transfer coordination, host-visible allocation preferences, and null application pipeline cache.
- **Limitations:** master snapshot; ROCmFPX may diverge and the project baseline is not frozen.

## S25-07 — HIP graphs

- **Publisher:** AMD
- **Document version:** HIP 7.2.53211 documentation
- **URL:** [HIP graphs](https://rocm.docs.amd.com/projects/HIP/en/latest/how-to/hip_runtime_api/hipgraph.html)
- **Supports:** graph nodes, stream capture, reuse model, external-semaphore nodes, and capture restrictions.
- **Limitations:** documentation version does not prove gfx1151 support or project performance.

## S25-08 — HIP memory management

- **Publisher:** AMD
- **Document version:** HIP 7.2.53211 documentation
- **URLs:** [Memory management](https://rocm.docs.amd.com/projects/HIP/en/latest/how-to/hip_runtime_api/memory_management.html), [Host memory](https://rocm.docs.amd.com/projects/HIP/en/develop/how-to/hip_runtime_api/memory_management/host_memory.html)
- **Supports:** pinned/mapped host memory and coherent, non-coherent, and write-combined allocation choices.
- **Limitations:** one linked page is the mutable development documentation (displayed as HIP 7.13.0 when accessed); freeze the installed HIP version before implementation.

## S25-09 — Vulkan validation layers

- **Publisher:** Khronos Group
- **Revision/date:** current Vulkan SDK documentation; accessed 2026-07-16
- **URL:** [Validation layers](https://vulkan.lunarg.com/doc/view/latest/linux/khronos_validation_layer.html)
- **Supports:** validation-layer purpose and configuration boundary.
- **Limitations:** SDK “latest” is mutable and validation cannot prove application-level data correctness.

## Source conflicts and freshness

- **[VERIFIED]** S25-03 describes current Mesa main, while the machines may run a released or distribution-patched build. Machine evidence takes precedence for exposed runtime properties; exact source/package lineage is required to explain differences.
- **[VERIFIED]** S25-06 describes upstream llama.cpp master, not yet the selected ROCmFPX fork commit. Re-audit after section 11 freezes lineage.
- **[RECOMMENDATION]** Review this register on any kernel, firmware, Mesa, Vulkan-Headers, llama.cpp/ROCmFPX, or HIP upgrade.

