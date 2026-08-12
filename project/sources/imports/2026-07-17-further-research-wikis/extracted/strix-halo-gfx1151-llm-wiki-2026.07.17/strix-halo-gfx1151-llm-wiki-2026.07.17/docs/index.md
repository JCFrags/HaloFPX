# Strix Halo / gfx1151 build and compatibility wiki

<span class="badge badge-version">Snapshot 2026.07.17</span> <span class="badge badge-target">gfx1151</span> <span class="badge badge-static">Static-validation release</span>

This wiki reconciles the Linux kernel, amdgpu firmware, ROCm/HIP/LLVM, Mesa/Vulkan, CMake, llama.cpp, ROCmFPX, and USB4 networking layers for AMD Strix Halo. It separates **official support**, **upstream release status**, **community validation**, **experimental work**, and **known-bad combinations**.

> [!IMPORTANT]
> A supported component does not make the complete stack supported. AMD’s ROCm 7.14 matrix establishes current **Core SDK** support for gfx1151, while AMD’s Ryzen application pages and RDNA 3.5 prebuilt-release table retain narrower release-specific scopes. See [Evidence model](evidence-model.md).

## Current conclusions

1. **Current official Core SDK lane:** ROCm 7.14 supports gfx1151 on Ubuntu 26.04 with kernel 7.0 GA and Ubuntu 24.04.4 with HWE kernel 6.17. The release line pairs HIP 7.14 with LLVM 23.0.0. This does not automatically certify every Ryzen application. Sources: [AMD-CORE-714](sources.md#amd-core-714), [AMD-INSTALL-714](sources.md#amd-install-714).
2. **Current official RDNA 3.5 prebuilt lane:** qualifying kernels are Ubuntu HWE `>=6.17.0-19.19~24.04.2`, Ubuntu OEM `>=6.14.0-1018`, or `>=6.18.4` elsewhere. AMD’s target-specific table marks ROCm 7.2.1–7.2.3 stable on those kernels. Source: [AMD-RDNA35](sources.md#amd-rdna35).
3. **Current maintained community lane:** Fedora 42/43, kernel 6.18.9, firmware 20260110, ROCm 7.2.4, and RADV are maintained as a known-good Strix Halo toolbox baseline. ROCm 7.2.4 is an official general release, but is not named in the captured gfx1151-specific 7.2.1–7.2.3 row. Sources: [KYUZ0-TOOLBOX-A7C71E9](sources.md#kyuz0-toolbox-a7c71e9), [AMD-CORE-724](sources.md#amd-core-724), [AMD-RDNA35](sources.md#amd-rdna35).
4. **Firmware is a hard gate:** avoid the `linux-firmware-20251125` / MES 0x83 era. A new kernel alone does not remove that firmware failure mode. Sources: [ROCM-ISSUE-5724](sources.md#rocm-issue-5724), [FEDORA-BUG-2420062](sources.md#fedora-bug-2420062), [LINUX-FW-REVERT-C092](sources.md#linux-fw-revert-c092).
5. **llama.cpp pin:** this snapshot uses release `b10064`, commit `86d86ed4396b4130922f7b9af26e3d9fc11a591b`. The current safe gfx1151 HIP baseline explicitly leaves rocWMMA FlashAttention OFF, matching upstream’s source default and the reported regression. Sources: [LLAMA-RELEASE-B10064](sources.md#llama-release-b10064), [LLAMA-GGML-CMAKE-86D86ED](sources.md#llama-ggml-cmake-86d86ed), [LLAMA-ISSUE-24437](sources.md#llama-issue-24437).
6. **Vulkan lane:** use Mesa RADV first for broad compatibility. AMDVLK has a community-reported large single-allocation limitation. Mesa 26.1.5 is the current upstream source pin, but the captured gfx1151 reports exercised 26.0.2/26.0.3. Sources: [MESA-2615](sources.md#mesa-2615), [KYUZ0-TOOLBOX-A7C71E9](sources.md#kyuz0-toolbox-a7c71e9), [LLAMA-ISSUE-15054](sources.md#llama-issue-15054).
7. **USB4:** standard IP networking uses upstream `CONFIG_USB4_NET` / `thunderbolt_net`. `thunderbolt-ibverbs` is a separate research driver explicitly described as buggy, insecure, and non-production. Sources: [LINUX-USB4-NET-KCONFIG](sources.md#linux-usb4-net-kconfig), [LINUX-THUNDERBOLT-DOC](sources.md#linux-thunderbolt-doc), [THUNDERBOLT-IBVERBS-76BA39B](sources.md#thunderbolt-ibverbs-76ba39b).

## Choose a lane

| Goal | Start here | Classification |
|---|---|---|
| Current AMD Core SDK | [ROCm 7.14 host and tarball recipe](recipes/host-official-rocm-714.md) | Official Core SDK + pinned build candidate |
| Conservative HIP llama.cpp | [ROCm 7.2.1 / b10064 source build](recipes/llama-hip-rocm721.md) | Official target release + upstream source |
| Maintained Fedora profile | [Fedora / ROCm 7.2.4 community recipe](recipes/community-fedora-rocm724.md) | Community-validated |
| Most compatible llama.cpp backend | [Mesa RADV / Vulkan build](recipes/llama-vulkan-radv.md) | Upstream + community-validated |
| Experimental AMD-focused quants | [ROCmFPX pinned build](recipes/rocmfpx-pinned.md) | Experimental community |
| Two-node IP link | [USB4/Thunderbolt IP recipe](recipes/usb4-ip-network.md) | Upstream kernel component |
| Research RDMA link | [USB4 verbs recipe](recipes/usb4-rdma-experimental.md) | Experimental, insecure |

## Release contents

- Human matrix: [Compatibility matrix](compatibility-matrix.md)
- Machine matrix: [`data/compatibility-matrix-2026.07.17.json`](../data/compatibility-matrix-2026.07.17.json), CSV, and YAML
- Exact flags: [Build flags](build-flags.md)
- Known failures: [Regressions](regressions.md) and [Unsupported combinations](unsupported-combinations.md)
- Runtime controls: [Environment variables](environment-variables.md)
- Collection and smoke tests: [Diagnostics](diagnostics.md)
- Reproducible images: [Containers](containers.md)
- Evidence: [Official support](official-support.md), [Community validation](community-validation.md), and [Source registry](sources.md)

## Validation boundary

The release generator validated data structure, script syntax, local links, and offline rendering. It did **not** execute ROCm, Vulkan, kernel, firmware, USB4, or performance tests on physical Strix Halo hardware. Every recipe therefore includes an acceptance gate rather than claiming local hardware certification.
