---
section_id: "23"
title: "Software compatibility facts and matrix"
status: "needs-machine-validation"
last_verified: "2026-08-12"
applies_to:
  repositories: ["HaloFPX", "ROCmFPX"]
  software_versions: ["ROCm 7.2.1", "ROCm 7.14.0", "Linux 7.2-rc2", "Mesa 26.1.5"]
  hardware_revisions: ["gfx1151 premise"]
related_sections: ["13", "17", "18", "19", "20", "37", "50"]
---

# Software compatibility facts and matrix

## Status vocabulary

- **Production-supported:** exact combination explicitly supported by the relevant vendor matrix.
- **Experimental:** upstream/pre-release or vendor-marked unstable combination requiring project qualification.
- **Backported:** required source patches applied outside the vendor tuple; provenance and config must be recorded.
- **Unverified:** components may individually support gfx1151, but the combination has no project evidence.

## Source-backed facts

| ID | Claim |
|---|---|
| 23-F01 | **[VERIFIED]** AMD Ryzen Linux matrix: ROCm 7.2.1 supports `gfx1150`/`gfx1151`, including Ryzen AI Max+ 395, on Ubuntu 24.04.4; PyTorch 2.9.1, Python 3.12, and FP16 are the production-validated framework tuple ([S23-01]). |
| 23-F02 | **[VERIFIED]** The broader ROCm 7.14.0 matrix lists Radeon 8060S/`gfx1151`, Ubuntu 24.04.4 HWE 6.17 and Ubuntu 26.04 kernel 7.0, using the inbox kernel driver ([S23-02]). |
| 23-F03 | **[VERIFIED]** AMD requires KFD fixes `7f26af7bf9b76c2c2a1a761aab5803e52be21eea` and `7445db6a7d5a0242d8214582b480600b266cba9e`; minimums are Ubuntu HWE `6.17.0-19.19~24.04.2`, Ubuntu OEM `6.14.0-1018`, or other distributions Linux `6.18.4` ([S23-03], [S23-04], [S23-05]). |
| 23-F04 | **[VERIFIED]** AMD marks other-distribution kernels below 6.18.4 unstable/experimental for several gfx1151 ROCm streams, and identifies Fedora 43, Ubuntu 26.04, and Arch 2026.02.01 as carrying the fixes in native packaging ([S23-03]). |
| 23-F05 | **[VERIFIED]** LLVM's AMDGPU backend recognizes `gfx1151`/`amdgpu11.51` ([S23-06]). Recognition is not proof that every library/kernel is optimized or correct. |
| 23-F06 | **[VERIFIED]** Linux 7.2-rc2 has USB4STREAM documentation and `thunderbolt-stream`; multiple streams may coexist with `thunderbolt-net` ([S23-07]). |
| 23-F07 | **[VERIFIED]** Mesa 26.1.5 is a 2026-07-15 bug-fix release and Mesa implements Vulkan 1.4, while the reported device API depends on the driver/hardware ([S23-08]). |
| 23-F08 | **[VERIFIED]** RADV is a userspace Vulkan driver (`libvulkan_radeon.so`) layered over the kernel-mode driver ([S23-09]). It is not ROCr/HSA/HIP and its version must be tracked separately. |
| 23-F09 | **[VERIFIED]** AMD records lower-than-expected performance for some LLM workloads on Ryzen AI Max+ 395 with ROCm 7.2.1 ([S23-10]). |

## Compatibility matrix

| Stack ID | Distribution / kernel | Compute userspace | USB4 data path | Status and exact boundary |
|---|---|---|---|---|
| 23-M01 | Ubuntu 24.04.4; HWE >= `6.17.0-19.19~24.04.2` or OEM >= `6.14.0-1018` | ROCm 7.2.1; PyTorch 2.9.1; Python 3.12; FP16 | USB4NET (`thunderbolt-net`) | **[VERIFIED] Production-supported compute tuple**; USB4NET is upstream, but project dual-link behavior remains unmeasured. |
| 23-M02 | Ubuntu 24.04.4 HWE 6.17 inbox driver | ROCm Core SDK 7.14.0 | USB4NET | **[VERIFIED] Official broader matrix listing**; **[OPEN]** HaloFPX/ROCmFPX runtime and library coverage. Do not infer the narrower PyTorch validation. |
| 23-M03 | Ubuntu 26.04 generic kernel 7.0 inbox driver | ROCm Core SDK 7.14.0 | USB4NET | **[VERIFIED] Official broader matrix listing**; **[OPEN]** project deployment and exact point releases. |
| 23-M04 | Fedora 43 / native kernel carrying KFD fixes | ROCm packages or AMD prebuilt binaries | USB4NET | **[VERIFIED] Fix availability**; **[UNVERIFIED]** AMD prebuilt-binary support and project runtime. |
| 23-M05 | Linux 7.2-rc2 or later pre-release build | ROCm 7.2.1 or 7.14.0 where installable | USB4STREAM + optional USB4NET | **[EXPERIMENTAL]** USB4STREAM exists upstream; combined gfx1151 compute tuple and two-link behavior are unverified. |
| 23-M06 | Supported distro kernel plus a backport of `thunderbolt-stream` | supported ROCm tuple | USB4STREAM | **[BACKPORTED/UNVERIFIED]** Must pin source commits/config and pass transport, suspend, error, and compute tests. |
| 23-M07 | Other distro < 6.18.4 without both KFD fixes | any gfx1151 ROCm | any | **[RECOMMENDATION] Reject** for production; AMD warns of queue-creation/memory-availability failures. |
| 23-M08 | CachyOS; running `7.1.3-1-cachyos` | ROCm packages 7.2.4; HIP `7.2.53211`; Mesa 26.1.4 | dual `thunderbolt-net` with MPTCP; no USB4STREAM module/device | **[MEASURED] Current cluster baseline** [S23-L01]; HSA/gfx1151 and deployed service readiness observed, but not vendor-supported or fully qualified for HaloFPX. |
| 23-M09 | CachyOS; running `7.1.3-1-cachyos` | ROCm 7.2.4-family packages; Mesa/RADV 26.1.4 | July dual-`thunderbolt-net`/MPTCP state retained; transport modules not re-audited | **[MEASURED] Latest broad project compute tuple, observed before the later 2026-08-12 incident** [S23-L02]; HSA/gfx1151 and the conventional UD-Q6 production baseline were healthy, but the tuple is not AMD's supported Ubuntu matrix and does not qualify current HaloFPX/ROCmFPX. |

M08 and the next two claims retain the exact 2026-07-17 wording for evidence
continuity. Their cluster-state scope is S23-L01; M09 and S23-L02 are the
2026-08-12 broad platform-inventory update.

**[MEASURED]** S23-L02's service PIDs, restart counters, and executable hashes
are historical before-state. After the later HMM/global-OOM incident, nimo-1
recovered as PID `3113343`, InvocationID
`0656332b63a140eab7214627baa43253`, `NRestarts=1`, and nimo-2 recovered as
PID `2248760`, InvocationID `d15fe49610274e77bd9a3d84a0b791a5`,
`NRestarts=1`. Both units were active/running, coordinator health was OK, and
a real 5-prompt-token plus 1-generated-token request completed [S23-L03]. The
recovery receipt does not refresh executable or loaded-library hashes.

**[MEASURED]** Both nodes had GCC 16.1.1, Clang 22.1.6, CMake 4.3.4, Ninja 1.13.2, Python 3.14.6, and linux-firmware 20260622. Package parity was not exact: nimo-1 exposed `hipcub` and the aggregate `rocm-hip-sdk` package in the captured subset, and installed LTS-kernel package revisions differed by one point [S23-L01].

**[MEASURED]** `/dev/kfd` and `/dev/dri/renderD128` were mode `0666` on both hosts [S23-L01]. **[RECOMMENDATION]** Treat this as a permissions-policy review item before multi-user service or broader network exposure; device availability alone does not justify world-writable accelerator nodes.

## Firmware and Mesa boundary

**[MEASURED]** The live package snapshot pins linux-firmware to 20260622 and Mesa to 26.1.4 [S23-L01]. **[OPEN]** Loaded firmware file hashes, amdgpu load log, exact Mesa build flags, RADV API, and extension inventory still require capture. The rolling upstream firmware repository ([S23-11]) is not a substitute for those files.

**[MEASURED]** The superseding 2026-08-12 live package snapshot reported `linux-firmware 1:20260622-1`, `mesa 3:26.1.4-1`, and `vulkan-radeon 3:26.1.4-1` [S23-L02]. The open loaded-firmware, build-flag, API, and extension qualification items remain unchanged.

**[RECOMMENDATION]** HIP and Vulkan are independent backends. Record `rocminfo`/HIP results separately from `vulkaninfo`; success in one does not validate the other.

[S23-01]: sources.md#s23-01
[S23-02]: sources.md#s23-02
[S23-03]: sources.md#s23-03
[S23-04]: sources.md#s23-04
[S23-05]: sources.md#s23-05
[S23-06]: sources.md#s23-06
[S23-07]: sources.md#s23-07
[S23-08]: sources.md#s23-08
[S23-09]: sources.md#s23-09
[S23-10]: sources.md#s23-10
[S23-11]: sources.md#s23-11
[S23-L03]: sources.md#s23-l03
