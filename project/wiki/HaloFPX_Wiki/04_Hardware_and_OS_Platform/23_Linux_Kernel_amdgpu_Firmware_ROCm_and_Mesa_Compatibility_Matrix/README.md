---
section_id: "23"
title: "Linux, Kernel, amdgpu, Firmware, ROCm, and Mesa Compatibility Matrix"
status: "needs-machine-validation"
last_verified: "2026-08-12"
applies_to:
  repositories: ["HaloFPX", "ROCmFPX"]
  software_versions: ["CachyOS Linux 7.1.3-1-cachyos", "ROCm 7.2.4", "Mesa 26.1.4", "ROCm 7.2.1", "ROCm 7.14.0", "Linux 7.2-rc2", "Mesa 26.1.5"]
  hardware_revisions: ["Nimo Direct MME3L; Ryzen AI MAX+ 395; Radeon 8060S/gfx1151"]
related_sections: ["13", "17", "18", "19", "20", "37", "50", "70", "81"]
---

# 23 - Linux, Kernel, amdgpu, Firmware, ROCm, and Mesa Compatibility Matrix

## High-value conclusion

**[VERIFIED]** AMD's Ryzen-specific Linux matrix gives production support for Ryzen AI Max+ 395 (`gfx1151`) with ROCm 7.2.1 on Ubuntu 24.04.4 and validates PyTorch 2.9.1/Python 3.12/FP16 ([S23-01]). AMD separately documents mandatory KFD fixes and minimum kernels for gfx1151 ([S23-03]).

**[VERIFIED]** Upstream Linux 7.2-rc2 documents USB4STREAM through `thunderbolt-stream` and `/dev/tbstreamX` ([S23-07]). This is not yet a production kernel release and is outside the Ryzen-specific ROCm 7.2.1 support tuple. Therefore the combined HaloFPX compute + USB4STREAM stack is **[OPEN]**, not “verified compatible.”

The next two claims are preserved verbatim from the 2026-07-17 state. Their
use of “currently” is capture-scoped and is superseded for present authority by
the 2026-08-12 block below.

**[RECOMMENDATION]** Keep two lanes until measured: (A) supported compute baseline using AMD's exact tuple and USB4NET; (B) experimental Linux 7.2 USB4STREAM qualification with the same userspace where installable. Do not call a backport production-supported.

**[MEASURED]** Both target machines currently run CachyOS kernel `7.1.3-1-cachyos`, ROCm 7.2.4 packages, Mesa 26.1.4, and linux-firmware 20260622; HSA enumerates gfx1151 and the deployed ROCmFP4 service is healthy, but this is an experimental project tuple rather than AMD's production-supported Ubuntu matrix [S23-L01]. The current kernel exposes USB4NET but no USB4STREAM module/device.

### 2026-08-12 superseding platform and service authority

**[RECOMMENDATION]** Keep three clearly named lanes: (A) the measured project
target, CachyOS, with USB4NET retained from the July transport capture pending
requalification; (B) AMD's supported Ubuntu compute tuple as a
portability/control lane; and (C) experimental Linux 7.2 USB4STREAM
qualification. Do not imply that Ubuntu is installed on the project targets or
call a backport production-supported.

**[MEASURED]** A read-only audit on 2026-08-12 observed both Nimo MME3L targets running CachyOS kernel `7.1.3-1-cachyos`, ROCm 7.2.4-family packages, Mesa/RADV 26.1.4, and linux-firmware 20260622; HSA enumerated gfx1151 [S23-L02]. nimo-1 was the active conventional UD-Q6 coordinator/API service and nimo-2 its RPC worker. This remains the latest broad project-tuple inventory, not AMD's supported Ubuntu tuple and not a deployed ROCmFPX result. The July 17 ROCmFP4 deployment in S23-L01 is historical.

**[MEASURED]** S23-L02's zero-restart service identities and executable hashes
are pre-incident observations. A later HMM/global-OOM incident recovered
nimo-1 as PID `3113343`, InvocationID
`0656332b63a140eab7214627baa43253`, `NRestarts=1`, and nimo-2 as PID
`2248760`, InvocationID `d15fe49610274e77bd9a3d84a0b791a5`,
`NRestarts=1`. Both units were active/running, coordinator health was OK, and
a real 5-prompt-token plus 1-generated-token request completed [S23-L03].
S23-L03 supersedes service identity and recovery state only; it did not rehash
either executable or loaded libraries and is not a performance result.

## Pages

- [Facts and compatibility matrix](facts_and_constraints.md)
- [Design implications](design_implications.md)
- [Procedures and checks](procedures_and_checks.md)
- [Open questions](open_questions.md)
- [Sources](sources.md)

## Research split

1. **Internet/source research complete:** official ROCm tuples, required KFD fixes, compiler target, current Mesa release, RADV boundary, and upstream USB4STREAM status.
2. **Machine work required:** kernel config/source ancestry, PCI and loaded-firmware hashes, independent HIP/Vulkan correctness, USB4STREAM, sustained workload stability, and reproducible bare-metal recovery.
3. **Contingent decisions:** production distribution/kernel, inbox vs packaged driver, ROCm release, firmware pin, Mesa pin, and USB4STREAM/backport strategy.

[S23-01]: sources.md#s23-01
[S23-03]: sources.md#s23-03
[S23-07]: sources.md#s23-07
[S23-L02]: sources.md#s23-l02
[S23-L03]: sources.md#s23-l03
