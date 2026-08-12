---
section_id: "19"
title: "Unified-memory sources"
status: "needs-machine-validation"
last_verified: "2026-07-17"
applies_to:
  repositories: []
  software_versions: ["ROCm 7.14 documentation", "Linux kernel documentation"]
  hardware_revisions: ["Radeon 8060S / gfx1151"]
related_sections: ["17", "23", "24", "54"]
---

# Sources

Web sources accessed 2026-07-16.

| ID | Primary source / revision | Claims supported | Limitations |
|---|---|---|---|
| S19-01 | AMD ROCm 7.14, [GPU specifications](https://rocm.docs.amd.com/en/latest/reference/gpu-specs.html), page dated 2026-02-20 | gfx1151/RDNA3.5, 40 CU, dynamic + carve-out | Specification, not machine state |
| S19-02 | Linux kernel, [AMDGPU core driver infrastructure](https://docs.kernel.org/gpu/amdgpu/driver-core.html), current docs | GPUVM per-process tables and mapped page types | Kernel-version details can differ |
| S19-03 | Linux kernel, [DRM memory management / TTM](https://docs.kernel.org/gpu/drm-mm.html), current docs | TTM BO lifetime, movement, CPU mappings | General subsystem documentation |
| S19-04 | AMD ROCm 7.14, [RDNA3.5 system optimization](https://rocm.docs.amd.com/en/latest/how-to/system-optimization/rdna3-5.html), accessed 2026-07-16 | small carve-out guidance, TTM page limit, gfx1151 kernel requirements | Recommendation must be validated per machine |
| S19-05 | AMD HIP, [device memory](https://rocm.docs.amd.com/projects/HIP/en/latest/how-to/hip_runtime_api/memory_management/device_memory.html) | APUs can share CPU/GPU physical memory | API-general |
| S19-06 | AMD HIP 7.2.53211, [memory management](https://rocm.docs.amd.com/projects/HIP/en/latest/how-to/hip_runtime_api/memory_management.html) and [unified memory](https://rocm.docs.amd.com/projects/HIP/en/docs-7.2.2/how-to/hip_runtime_api/memory_management/unified_memory.html) | allocators, pinned/managed behavior, capabilities | Runtime support is device/system dependent |
| S19-07 | AMD HIP 7.2.53211, [coherence control](https://rocm.docs.amd.com/projects/HIP/en/latest/how-to/hip_runtime_api/memory_management/coherence_control.html) | fine/coarse semantics and tradeoffs | Measure on target |
| S19-08 | Linux kernel 6.3, [amdgpu module parameters](https://docs.kernel.org/6.3/gpu/amdgpu/module-parameters.html) | `no_system_mem_limit` semantics | Older versioned doc; verify source for selected kernel |

## Local primary observations

- **S19-L01:** preserved [nimo-1 audit](../../../../sources/measurements/2026-07-10_12-strix-halo-cluster/redacted-audits/2026-07-12__nimo-1__deep-system-audit__v01.md), capture 2026-07-12, SHA-256 `03982946a2eb8fd18d6117861c5e4c75f43986fb366a1da5b57416f5ab2a50f2`. Supports what the historical node-1 report records about memory/kernel/IOMMU state; synthesized/redacted, not raw current state.
- **S19-L02:** preserved [nimo-2 audit](../../../../sources/measurements/2026-07-10_12-strix-halo-cluster/redacted-audits/2026-07-12__nimo-2__deep-system-audit__v01.md), SHA-256 `ecdc400942a1ed95615aeaddc83d2c78e2c38a9fcdcc0b56a68a77468b26e410`. Supports the historical peer report; same limitations.
- **S19-L03:** [2026-07-17 live matched-pair inventory](../../../../sources/measurements/2026-07-17-strix-halo-live-inventory/README.md). Supports current memory, NUMA, boot parameter, GTT, swap, and active-process accounting. It is a point-in-time normalized capture and does not replace allocator or stress experiments.
