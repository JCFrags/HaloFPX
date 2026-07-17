---
section_id: "23"
title: "Software stack design implications"
status: "needs-machine-validation"
last_verified: "2026-07-16"
applies_to:
  repositories: ["HaloFPX", "ROCmFPX"]
  software_versions: ["ROCm 7.2.1", "ROCm 7.14.0", "Linux 7.2-rc2", "Mesa 26.1.5"]
  hardware_revisions: ["gfx1151 premise"]
related_sections: ["13", "19", "20", "37", "50", "70", "81"]
---

# Software stack design implications

## Two-lane platform policy

- **[RECOMMENDATION] Supported lane:** Ubuntu 24.04.4 with AMD's minimum fixed HWE/OEM kernel, ROCm 7.2.1 exact packages, and USB4NET. Use it as the control for correctness and rollback.
- **[RECOMMENDATION] Experimental lane:** pinned Linux 7.2 source/commit with `thunderbolt-stream`, exact config, identical firmware/userspace where compatible, and a separate boot entry. Never overwrite the known-good kernel.
- **[RECOMMENDATION]** Qualify ROCm 7.14.0 as its own lane because its broader matrix is newer/different from the Ryzen-specific 7.2.1 framework matrix.

## Reproducibility contract

Every binary/result must identify:

1. distribution release and repository snapshot;
2. `uname -r`, kernel build ID/config/source commit, and both KFD-fix ancestry checks;
3. amdgpu module version/source and parameters;
4. linux-firmware package plus loaded-file hashes;
5. ROCm package versions, HIP compiler version/target, HSA runtime/agents;
6. Mesa/RADV version, Vulkan loader/ICD path, API and required extensions;
7. HaloFPX/ROCmFPX/llama.cpp source commits and build flags;
8. USB4 controller domains, driver modules, security/IOMMU state, and per-link device/interface mapping.

## Build and runtime gates

- **[RECOMMENDATION]** Fail configuration if `gfx1151` is absent from compiler targets or if runtime agent identity differs from the build target.
- **[RECOMMENDATION]** Run HIP allocation/kernel/copy tests and Vulkan buffer/compute tests independently before inference.
- **[RECOMMENDATION]** Treat missing/corrupt firmware, GPU reset, KFD queue failure, HSA enumeration failure, or wrong target as a hard readiness failure.
- **[RECOMMENDATION]** Validate single-node inference before distributed mode. A transport or peer failure must have an explicit single-node fallback.
- **[INFERENCE]** A backported USB4STREAM driver increases maintenance and failure-surface cost; it is worthwhile only if end-to-end GPU-to-peer benefits exceed upstream USB4NET on matched tests.

## Mesa/RADV scope

**[RECOMMENDATION]** Pin Mesa only for Vulkan builds. Do not require a Mesa update to solve a HIP/HSA problem without evidence. Conversely, a supported ROCm tuple does not guarantee the Vulkan extensions or RADV behavior required by a particular llama.cpp commit.

## Upgrade policy

Promote a stack only after paired-node inventory, boot/reboot/suspend, HSA/HIP/Vulkan smoke, long inference, GPU reset/error review, both USB4 links independently and together, and rollback boot. Preserve the previously verified kernel/packages/firmware manifest.
