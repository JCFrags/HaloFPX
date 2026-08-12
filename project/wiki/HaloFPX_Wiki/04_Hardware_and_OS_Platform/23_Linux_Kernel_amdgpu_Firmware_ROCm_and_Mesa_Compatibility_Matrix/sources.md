---
section_id: "23"
title: "Software compatibility sources"
status: "draft"
last_verified: "2026-07-17"
applies_to:
  repositories: ["HaloFPX", "ROCmFPX"]
  software_versions: []
  hardware_revisions: []
related_sections: ["13", "17", "18", "19", "20", "37", "50"]
---

# Software compatibility sources

## S23-L01 — Live target software and accelerator inventory

- **Canonical source:** [`../../../../sources/measurements/2026-07-17-strix-halo-live-inventory/README.md`](../../../../sources/measurements/2026-07-17-strix-halo-live-inventory/README.md)
- **Capture:** both nodes, 2026-07-17 11:52–12:05 America/Los_Angeles.
- **Supports:** current kernel, ROCm/HIP, Mesa, firmware, toolchain, gfx1151 enumeration, device permissions, installed package skew, and USB4NET/USB4STREAM capability state.
- **Limitations:** does not establish vendor support, kernel-fix ancestry, Vulkan correctness, reproducible build closure, or sustained inference stability.

## S23-01

- **Title/publisher:** Linux support matrices by ROCm version - Ryzen / AMD
- **URL:** https://rocm.docs.amd.com/projects/radeon-ryzen/en/latest/docs/compatibility/compatibilityryz/native_linux/native_linux_compatibility.html
- **Revision/date/access:** ROCm 7.2.1 matrix; accessed 2026-07-16
- **Supports:** Ubuntu 24.04.4, gfx1151/Max+ 395, PyTorch 2.9.1, Python 3.12, FP16 production tuple.
- **Limitations:** Framework validation is narrow; does not validate HaloFPX, USB4STREAM, or all ROCm libraries.

## S23-02

- **Title/publisher:** ROCm 7.14.0 compatibility matrix / AMD
- **URL:** https://rocm.docs.amd.com/en/latest/compatibility/compatibility-matrix.html
- **Revision/date/access:** ROCm 7.14.0; accessed 2026-07-16
- **Supports:** Radeon 8060S/gfx1151, Ubuntu 24.04.4 HWE 6.17, Ubuntu 26.04 kernel 7.0, inbox driver.
- **Limitations:** Broader SDK matrix; do not import Ryzen-specific framework validation from S23-01.

## S23-03

- **Title/publisher:** AMD RDNA3.5 system optimization / AMD ROCm
- **URL:** https://rocm.docs.amd.com/en/latest/reference/system-optimization/rdna3-5.html
- **Revision/date/access:** ROCm 7.14.0 docs; accessed 2026-07-16
- **Supports:** required KFD fixes, minimum kernel versions, supported/experimental release table, native distro fix availability.
- **Limitations:** AMD prebuilt-binary table; native packages can differ.

## S23-04

- **Title/repository:** `drm/amdkfd: bump minimum vgpr size for gfx1151` / Linux stable tree
- **URL:** https://github.com/gregkh/linux/commit/7f26af7bf9b76c2c2a1a761aab5803e52be21eea
- **Commit/access:** `7f26af7bf9b76c2c2a1a761aab5803e52be21eea`; accessed 2026-07-16
- **Supports:** gfx1151 VGPR availability fix for KFD queue checks.
- **Limitations:** A commit alone does not establish a distribution kernel contains it.

## S23-05

- **Title/repository:** `drm/amdkfd: Export the cwsr_size and ctl_stack_size to userspace` / Linux stable tree
- **URL:** https://github.com/gregkh/linux/commit/7445db6a7d5a0242d8214582b480600b266cba9e
- **Commit/access:** `7445db6a7d5a0242d8214582b480600b266cba9e`; accessed 2026-07-16
- **Supports:** exporting sizes so userspace need not hard-code VGPR-related state.
- **Limitations:** Same backport/ancestry boundary as S23-04.

## S23-06

- **Title/publisher:** User Guide for AMDGPU Backend / LLVM project
- **URL:** https://llvm.org/docs/AMDGPUUsage.html
- **Revision/date/access:** current docs accessed 2026-07-16
- **Supports:** `gfx1151` / `amdgpu11.51` target recognition.
- **Limitations:** Target recognition is not whole-stack validation.

## S23-07

- **Title/publisher:** USB4 and Thunderbolt / Linux kernel documentation
- **URL:** https://docs.kernel.org/admin-guide/thunderbolt.html
- **Revision/date/access:** Linux 7.2.0-rc2 docs; accessed 2026-07-16
- **Supports:** USB4NET and USB4STREAM module, ConfigFS, device nodes, multi-stream coexistence.
- **Limitations:** Pre-release kernel documentation; no project hardware/performance proof.

## S23-08

- **Title/publisher:** Mesa 26.1.5 release notes / Mesa project
- **URL:** https://docs.mesa3d.org/relnotes/26.1.5.html
- **Revision/date/access:** 26.1.5, 2026-07-15; accessed 2026-07-16
- **Supports:** current bug-fix release/date/checksum and API-reporting caveat.
- **Limitations:** No per-gfx1151 extension or project workload validation.

## S23-09

- **Title/publisher:** RADV driver documentation / Mesa project
- **URL:** https://docs.mesa3d.org/drivers/radv.html
- **Revision/date/access:** current docs accessed 2026-07-16
- **Supports:** RADV userspace/KMD responsibilities and library identity.
- **Limitations:** Actual driver selection and device capabilities require live evidence.

## S23-10

- **Title/publisher:** Ryzen limitations and recommended settings / AMD ROCm
- **URL:** https://rocm.docs.amd.com/projects/radeon-ryzen/en/latest/docs/limitations/limitationsryz.html
- **Revision/date/access:** ROCm 7.2.1 limitations; accessed 2026-07-16
- **Supports:** known lower-than-expected LLM performance on Ryzen AI Max+ 395.
- **Limitations:** Broad issue statement; exact affected models/configurations and fix status need current release comparison.

## S23-11

- **Title/repository:** linux-firmware / kernel.org
- **URL:** https://git.kernel.org/pub/scm/linux/kernel/git/firmware/linux-firmware.git/
- **Revision/date/access:** rolling repository; accessed 2026-07-16
- **Supports:** canonical firmware source and commit-level provenance.
- **Limitations:** The repository head is not evidence of files loaded on either node.
