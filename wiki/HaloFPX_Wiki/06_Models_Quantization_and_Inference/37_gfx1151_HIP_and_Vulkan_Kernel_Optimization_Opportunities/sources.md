---
section_id: "37"
title: "gfx1151 optimization sources"
status: "draft"
last_verified: "2026-07-16"
applies_to:
  repositories: ["charlie12345/ROCmFPX@a5605a72768c6562241b248e268e33dc92787394", "ggml-org/llama.cpp@788e07dc91d266ad3162a1ce9037665656269689"]
  software_versions: []
  hardware_revisions: ["gfx1151"]
related_sections: ["24", "25", "27"]
---

# Sources

| ID | Primary source and revision | Supports | Limitations |
|---|---|---|---|
| S37-01 | [ROCmFPX source](https://github.com/charlie12345/ROCmFPX/tree/a5605a72768c6562241b248e268e33dc92787394), commit `a5605a72`, committed 2026-07-16 -04:00, accessed 2026-07-16 PDT | custom HIP/Vulkan kernels, build notes, experiment leads | repository measurements are environment-specific |
| S37-02 | [llama.cpp source](https://github.com/ggml-org/llama.cpp/tree/788e07dc91d266ad3162a1ce9037665656269689), commit `788e07dc`, committed 2026-07-17 +02:00, accessed 2026-07-16 PDT | current backend/kernel/graph baseline | moving project; HaloFPX fork differs |
| S37-03 | [AMD RDNA3.5 ISA reference](https://docs.amd.com/v/u/en-US/rdna35_instruction_set_architecture), document 70649, release 2024-07-23 | architecture instruction/state authority | not a performance guide or compiler guarantee |
| S37-04 | [Vulkan subgroup guide](https://docs.vulkan.org/guide/latest/subgroups.html), accessed 2026-07-16 | subgroup queries and size control | implementation-specific capabilities remain unmeasured |
| S37-05 | [ROCprofiler-SDK documentation](https://rocm.docs.amd.com/projects/rocprofiler-sdk/en/develop/), accessed 2026-07-16 | activity tracing and counter collection | develop docs can change; pin installed tool version |
| S37-06 | [Vulkan compute shader guide](https://docs.vulkan.org/guide/latest/compute_shaders.html), accessed 2026-07-16 | workgroup/subgroup/shared-memory concepts | generic, not AMD-specific |
| S37-07 | [rocWMMA documentation](https://rocm.docs.amd.com/projects/rocwmma/en/latest/), accessed 2026-07-16 | wave-centric matrix API and supported-family checks | actual gfx1151/type support must be verified for pinned release |

Source count is 7.
