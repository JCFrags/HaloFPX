---
section_id: "54"
title: "Buffer-path sources"
status: "needs-machine-validation"
last_verified: "2026-07-16"
applies_to:
  repositories: ["torvalds/linux@8cdeaa50eae8dad34885515f62559ee83e7e8dda"]
  software_versions: ["Linux 7.2-rc2 USB4STREAM", "HIP 7.2.53211 documentation", "Vulkan 1.4.356"]
  hardware_revisions: ["Radeon 8060S/gfx1151"]
related_sections: ["19", "24", "25", "50", "55"]
---

# Sources

Web sources accessed 2026-07-16.

| ID | Primary source / revision | Supports | Limitations |
|---|---|---|---|
| S54-01 | AMD HIP 7.2.53211, [memory management](https://rocm.docs.amd.com/projects/HIP/en/latest/how-to/hip_runtime_api/memory_management.html) and [host-allocation flags](https://rocm.docs.amd.com/projects/HIP/en/latest/doxygen/html/group___global_defs.html) | pinned/mapped allocation APIs | Actual gfx1151 behavior must be measured |
| S54-02 | AMD HIP 7.2.53211, [coherence control](https://rocm.docs.amd.com/projects/HIP/en/latest/how-to/hip_runtime_api/memory_management/coherence_control.html) | fine/coarse semantics and visibility | Mutable latest docs; pin runtime headers/source at implementation |
| S54-03 | Khronos, [Vulkan 1.4.356 specification](https://registry.khronos.org/vulkan/specs/1.4-extensions/html/vkspec.html), published 2026-07 | heaps/types, host visibility/coherence, barriers/fences | Implementation capabilities are queried, not assumed |
| S54-04 | Linux kernel, [dma-buf sharing and synchronization](https://docs.kernel.org/6.16/driver-api/dma-buf.html), Linux 6.16 docs | exporter/importer, fences, CPU synchronization, mmap | General subsystem; USB4STREAM has no importer |
| S54-05 | Linux man-pages/liburing, [registered buffers overview](https://man7.org/linux/man-pages/man7/io_uring_registered_buffers.7.html), accessed 2026-07-16; liburing latest release 2.14 | long-term registered mappings and fixed-buffer operations | Availability/behavior depends on kernel/file operation; not a zero-copy promise |
| S54-06 | Linux v7.2-rc2 commit `8cdeaa50eae8dad34885515f62559ee83e7e8dda`, [`drivers/thunderbolt/stream.c`](https://github.com/torvalds/linux/blob/v7.2-rc2/drivers/thunderbolt/stream.c) | exact USB4STREAM file ops, page copies, frames/rings | Release candidate; not running on project nodes |

## Scoped project evidence

- **S54-L01:** `C:/Users/britt/AgentRoot/10_projects/11_active/11.02_strix-halo-usb4-cluster/01_discovery/output/2026-07-12__m2-usb4stream-transport__feasibility-plan__v01.md`, SHA-256 subject recorded by D-035. Supports source-derived current project boundary and tests; plan, not execution.
- **S54-L02:** `C:/Users/britt/AgentRoot/10_projects/11_active/11.02_strix-halo-usb4-cluster/DECISIONS.md`, D-2026-07-12-035 accepted 2026-07-12. Governs USB4NET default/reversible probe; no performance result.
