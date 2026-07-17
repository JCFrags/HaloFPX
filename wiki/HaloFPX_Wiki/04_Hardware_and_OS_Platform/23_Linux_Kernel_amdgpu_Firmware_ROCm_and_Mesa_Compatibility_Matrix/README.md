---
section_id: "23"
title: "Linux, Kernel, amdgpu, Firmware, ROCm, and Mesa Compatibility Matrix"
status: "needs-machine-validation"
last_verified: "2026-07-17"
applies_to:
  repositories: ["HaloFPX", "ROCmFPX"]
  software_versions: ["ROCm 7.2.1", "ROCm 7.14.0", "Linux 7.2-rc2", "Mesa 26.1.5"]
  hardware_revisions: ["Radeon 8060S/gfx1151 premise; exact systems open"]
related_sections: ["13", "17", "18", "19", "20", "37", "50", "70", "81"]
---

# 23 - Linux, Kernel, amdgpu, Firmware, ROCm, and Mesa Compatibility Matrix

## High-value conclusion

**[VERIFIED]** AMD's Ryzen-specific Linux matrix gives production support for Ryzen AI Max+ 395 (`gfx1151`) with ROCm 7.2.1 on Ubuntu 24.04.4 and validates PyTorch 2.9.1/Python 3.12/FP16 ([S23-01]). AMD separately documents mandatory KFD fixes and minimum kernels for gfx1151 ([S23-03]).

**[VERIFIED]** Upstream Linux 7.2-rc2 documents USB4STREAM through `thunderbolt-stream` and `/dev/tbstreamX` ([S23-07]). This is not yet a production kernel release and is outside the Ryzen-specific ROCm 7.2.1 support tuple. Therefore the combined HaloFPX compute + USB4STREAM stack is **[OPEN]**, not “verified compatible.”

**[RECOMMENDATION]** Keep two lanes until measured: (A) supported compute baseline using AMD's exact tuple and USB4NET; (B) experimental Linux 7.2 USB4STREAM qualification with the same userspace where installable. Do not call a backport production-supported.

**[MEASURED]** Both target machines currently run CachyOS kernel `7.1.3-1-cachyos`, ROCm 7.2.4 packages, Mesa 26.1.4, and linux-firmware 20260622; HSA enumerates gfx1151 and the deployed ROCmFP4 service is healthy, but this is an experimental project tuple rather than AMD's production-supported Ubuntu matrix [S23-L01]. The current kernel exposes USB4NET but no USB4STREAM module/device.

## Pages

- [Facts and compatibility matrix](facts_and_constraints.md)
- [Design implications](design_implications.md)
- [Procedures and checks](procedures_and_checks.md)
- [Open questions](open_questions.md)
- [Sources](sources.md)

## Research split

1. **Internet/source research complete:** official ROCm tuples, required KFD fixes, compiler target, current Mesa release, RADV boundary, and upstream USB4STREAM status.
2. **Machine work required:** actual OS/kernel/config, PCI IDs, firmware hashes, KFD/HSA/HIP behavior, Mesa/RADV extensions, USB4STREAM, and workload stability.
3. **Contingent decisions:** production distribution/kernel, inbox vs packaged driver, ROCm release, firmware pin, Mesa pin, and USB4STREAM/backport strategy.

[S23-01]: sources.md#s23-01
[S23-03]: sources.md#s23-03
[S23-07]: sources.md#s23-07
