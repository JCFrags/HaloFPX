# Source registry

This page is the human-readable form of [`sources/source-registry.json`](../sources/source-registry.json). Source IDs are stable inside version `2026.07.17`. A source proves only the scope stated here; support claims are not transitively combined.

## AMD-CORE-714

**ROCm 7.14 compatibility matrix** — AMD ROCm Documentation  
Classification: `official` · Type: `official-support-matrix` · Accessed: `2026-07-17`  
Scope: ROCm Core SDK hardware, OS, and component compatibility  
Source: <https://rocm.docs.amd.com/en/latest/compatibility/compatibility-matrix.html>

Claims used in this snapshot:

- gfx1151 is listed as a supported Ryzen GPU target for ROCm 7.14 Core SDK.
- Ubuntu 26.04 GA kernel 7.0 and Ubuntu 24.04.4 HWE kernel 6.17 are listed with the inbox kernel driver.
- ROCm/HIP 7.14 pairs with LLVM 23.0.0 and ROCr 1.21.0.

## AMD-INSTALL-714

**ROCm 7.14 installation guide and gfx1151 tarball** — AMD ROCm Documentation  
Classification: `official` · Type: `official-install-guide` · Accessed: `2026-07-17`  
Scope: ROCm SDK installation  
Source: <https://rocm.docs.amd.com/en/latest/install/rocm.html>

Claims used in this snapshot:

- Provides the versioned gfx1151 TheRock tarball URL.
- Documents ROCM_PATH, PATH, and LD_LIBRARY_PATH setup for tarball installs.

## AMD-RDNA35

**RDNA 3.5 / gfx115x system optimization and release support table** — AMD ROCm Documentation  
Classification: `official` · Type: `official-target-support-table` · Accessed: `2026-07-17`  
Scope: gfx1150/gfx1151 kernel requirements, release status, unified memory  
Source: <https://rocm.docs.amd.com/en/latest/reference/system-optimization/rdna3-5.html>

Claims used in this snapshot:

- Fixed minimum kernels are Ubuntu HWE 6.17.0-19.19~24.04.2, Ubuntu OEM 6.14.0-1018, or 6.18.4 on other distributions.
- ROCm 7.2.1 through 7.2.3 are marked stable on qualifying kernels; 7.1.x and 6.4.x are not supported there.
- Documents amd-ttm and GTT/TTM memory tuning guidance.

## AMD-CORE-724

**ROCm 7.2.4 compatibility matrix** — AMD ROCm Documentation  
Classification: `official` · Type: `official-release-matrix` · Accessed: `2026-07-17`  
Scope: General ROCm 7.2.4 components and operating systems  
Source: <https://rocm.docs.amd.com/en/docs-7.2.4/compatibility/compatibility-matrix.html>

Claims used in this snapshot:

- HIP 7.2.53211, LLVM 22.0.0.26084, ROCr 1.18.0, and ROCm CMake 0.14.0 are the 7.2.4 component line.
- This general matrix does not itself establish gfx1151 application validation.

## AMD-RYZEN-72

**ROCm 7.2 native Linux compatibility for Radeon and Ryzen** — AMD ROCm Documentation  
Classification: `official` · Type: `official-application-support` · Accessed: `2026-07-17`  
Scope: Ryzen gfx1150/gfx1151 application validation  
Source: <https://rocm.docs.amd.com/projects/radeon-ryzen/en/docs-7.2/docs/compatibility/compatibilityryz/native_linux/native_linux_compatibility.html>

Claims used in this snapshot:

- gfx1151 is included in the ROCm 7.2 Ryzen application-support line.
- Ubuntu 24.04.3 support is described as preliminary through the 24.04.2 installer path.
- PyTorch 2.9 with ROCm 7.2 and Python 3.12 is listed as production-supported.

## AMD-RYZEN-INDEX

**ROCm on Radeon and Ryzen documentation index** — AMD ROCm Documentation  
Classification: `official` · Type: `official-application-index` · Accessed: `2026-07-17`  
Scope: Radeon/Ryzen application use cases  
Source: <https://rocm.docs.amd.com/projects/radeon-ryzen/en/latest/index.html>

Claims used in this snapshot:

- llama.cpp is included among Radeon/Ryzen application use cases.

## AMD-LLAMA-711

**Historical AMD llama.cpp package for ROCm 7.1.1** — AMD ROCm Documentation  
Classification: `official-historical` · Type: `official-historical-recipe` · Accessed: `2026-07-17`  
Scope: Historical prebuilt gfx1150/gfx1151 llama.cpp bundle  
Source: <https://rocm.docs.amd.com/projects/radeon-ryzen/en/docs-7.1.1/docs/advanced/advancedryz/linux/llm/llamacpp.html>

Claims used in this snapshot:

- Documents llama-b7146 for Ubuntu 24.04, ROCm 7.1.1, gfx1150/gfx1151.

## LLAMA-RELEASE-B10064

**llama.cpp release b10064** — ggml-org  
Classification: `official-upstream` · Type: `upstream-release` · Accessed: `2026-07-17`  
Scope: Current pinned llama.cpp release and ROCm 7.2 binary asset  
Source: <https://github.com/ggml-org/llama.cpp/releases/tag/b10064>

Claims used in this snapshot:

- Release b10064 corresponds to source commit 86d86ed4396b4130922f7b9af26e3d9fc11a591b.
- Includes a Linux Ubuntu ROCm 7.2 x64 binary archive.

## LLAMA-HIP-CMAKE-86D86ED

**llama.cpp HIP backend CMake at 86d86ed** — ggml-org  
Classification: `official-upstream` · Type: `upstream-source` · Accessed: `2026-07-17`  
Scope: HIP compiler and target selection behavior  
Source: <https://github.com/ggml-org/llama.cpp/blob/86d86ed4396b4130922f7b9af26e3d9fc11a591b/ggml/src/ggml-hip/CMakeLists.txt>

Claims used in this snapshot:

- HIP 6.1 or newer is required.
- The native HIP-language path requires CMake 3.21 or newer.
- GPU_TARGETS and AMDGPU_TARGETS can be forwarded to CMAKE_HIP_ARCHITECTURES; direct CMAKE_HIP_ARCHITECTURES is preferred.

## LLAMA-GGML-CMAKE-86D86ED

**llama.cpp backend options at 86d86ed** — ggml-org  
Classification: `official-upstream` · Type: `upstream-source` · Accessed: `2026-07-17`  
Scope: Current ggml HIP and Vulkan option defaults  
Source: <https://github.com/ggml-org/llama.cpp/blob/86d86ed4396b4130922f7b9af26e3d9fc11a591b/ggml/CMakeLists.txt>

Claims used in this snapshot:

- GGML_HIP_GRAPHS defaults ON.
- GGML_HIP_NO_VMM defaults ON.
- GGML_HIP_ROCWMMA_FATTN defaults OFF.

## LLAMA-ROCM-DOCKER-86D86ED

**llama.cpp upstream ROCm Dockerfile at 86d86ed** — ggml-org  
Classification: `official-upstream` · Type: `upstream-container-recipe` · Accessed: `2026-07-17`  
Scope: Upstream ROCm build container  
Source: <https://github.com/ggml-org/llama.cpp/blob/86d86ed4396b4130922f7b9af26e3d9fc11a591b/.devops/rocm.Dockerfile>

Claims used in this snapshot:

- Defaults to Ubuntu 24.04 and ROCm 7.2.1.
- Uses the ROCm-bundled Clang and enables ROCm/HIP through CMake.

## LLAMA-ISSUE-24437

**gfx1151 rocWMMA FlashAttention prefill regression** — ggml-org issue tracker  
Classification: `community` · Type: `community-reproducible-regression` · Accessed: `2026-07-17`  
Scope: ROCm 7.2.1, gfx1151, long-context prefill  
Source: <https://github.com/ggml-org/llama.cpp/issues/24437>

Claims used in this snapshot:

- Enabling GGML_HIP_ROCWMMA_FATTN produced up to approximately 41% lower long-context prefill in the reported test.

## LLAMA-ISSUE-24438

**gfx1151 HIP versus Vulkan decode benchmark** — ggml-org issue tracker  
Classification: `community` · Type: `community-benchmark` · Accessed: `2026-07-17`  
Scope: ROCm 7.2.1 and Mesa 26.0.3 on Strix Halo  
Source: <https://github.com/ggml-org/llama.cpp/issues/24438>

Claims used in this snapshot:

- The reported Qwen3.6 35B-A3B Q8_0 decode workload favored Vulkan over HIP; this is workload-specific, not universal.

## LLAMA-ISSUE-22375

**RADV prompt-processing regression between llama.cpp builds** — ggml-org issue tracker  
Classification: `community` · Type: `community-reproducible-regression` · Accessed: `2026-07-17`  
Scope: Mesa 26.0.2 RADV, Linux 6.19.4  
Source: <https://github.com/ggml-org/llama.cpp/issues/22375>

Claims used in this snapshot:

- A reported b8460 to b8933 change reduced pp512 by roughly 39% while token generation remained stable.

## LLAMA-ISSUE-18741

**RADV model-load failure and direct-I/O workaround** — ggml-org issue tracker  
Classification: `community` · Type: `community-workaround` · Accessed: `2026-07-17`  
Scope: Vulkan/RADV model loading  
Source: <https://github.com/ggml-org/llama.cpp/issues/18741>

Claims used in this snapshot:

- --no-direct-io enabled model loading in the reported failure; --no-mmap is a common paired diagnostic.

## LLAMA-ISSUE-15054

**AMDVLK large single-allocation limitation** — ggml-org issue tracker  
Classification: `community` · Type: `community-driver-limitation` · Accessed: `2026-07-17`  
Scope: AMDVLK large-model loading  
Source: <https://github.com/ggml-org/llama.cpp/issues/15054>

Claims used in this snapshot:

- The report identifies an approximately 2 GiB single-buffer allocation limitation; the same model loaded under RADV.

## LLAMA-ISSUE-17014

**Historical ROCm 7.1 MoE performance and crash regression** — ggml-org issue tracker  
Classification: `community` · Type: `community-historical-regression` · Accessed: `2026-07-17`  
Scope: ROCm 7.1 and MoE workloads  
Source: <https://github.com/ggml-org/llama.cpp/issues/17014>

Claims used in this snapshot:

- Documents a ROCm 7.1-era performance/crash regression relevant to avoiding the obsolete stack.

## LLAMA-ISSUE-24961

**Related RDNA4 graph workaround** — ggml-org issue tracker  
Classification: `community` · Type: `community-related-diagnostic` · Accessed: `2026-07-17`  
Scope: RDNA4, not gfx1151-specific  
Source: <https://github.com/ggml-org/llama.cpp/issues/24961>

Claims used in this snapshot:

- GGML_CUDA_DISABLE_GRAPHS=1 is a related diagnostic only; it must not be treated as a default gfx1151 setting.

## MESA-2615

**Mesa 26.1.5 release notes** — Mesa Project  
Classification: `official-upstream` · Type: `upstream-release` · Accessed: `2026-07-17`  
Scope: Current Mesa bugfix release  
Source: <https://docs.mesa3d.org/relnotes/26.1.5.html>

Claims used in this snapshot:

- Mesa 26.1.5 was released 2026-07-15.
- The release tarball SHA-256 is 79e421c7ce18cd9e790b8375920325779f10798630bf30e0b22f1a21c8617122.

## CMAKE-HIP-ARCH

**CMAKE_HIP_ARCHITECTURES variable** — Kitware CMake Documentation  
Classification: `official-upstream` · Type: `upstream-build-documentation` · Accessed: `2026-07-17`  
Scope: HIP target selection  
Source: <https://cmake.org/cmake/help/latest/variable/CMAKE_HIP_ARCHITECTURES.html>

Claims used in this snapshot:

- CMAKE_HIP_ARCHITECTURES was added in CMake 3.21.

## CMAKE-RELEASE-434

**CMake v4.3.4 release** — Kitware  
Classification: `official-upstream` · Type: `upstream-release` · Accessed: `2026-07-17`  
Scope: Current CMake patch release  
Source: <https://github.com/Kitware/CMake/releases/tag/v4.3.4>

Claims used in this snapshot:

- CMake 4.3.4 is current as of the wiki snapshot, but is not assumed validated with every ROCm 7.2 recipe.

## VULKAN-LOADER-ICD

**Vulkan loader driver discovery interface** — KhronosGroup  
Classification: `official-upstream` · Type: `upstream-runtime-documentation` · Accessed: `2026-07-17`  
Scope: Vulkan ICD selection and diagnostics  
Source: <https://github.com/KhronosGroup/Vulkan-Loader/blob/main/docs/LoaderDriverInterface.md>

Claims used in this snapshot:

- VK_DRIVER_FILES selects manifest files; VK_ICD_FILENAMES is the older deprecated form.
- VK_LOADER_DEBUG enables loader diagnostics.

## LINUX-THUNDERBOLT-DOC

**Linux Thunderbolt and USB4 administration guide** — Linux Kernel Documentation  
Classification: `official-upstream` · Type: `upstream-kernel-documentation` · Accessed: `2026-07-17`  
Scope: Thunderbolt/USB4 networking  
Source: <https://docs.kernel.org/admin-guide/thunderbolt.html>

Claims used in this snapshot:

- Linux supports ThunderboltIP; thunderbolt-net can be manually loaded on one Linux peer.

## LINUX-USB4-NET-KCONFIG

**Linux CONFIG_USB4_NET definition** — Linux Kernel  
Classification: `official-upstream` · Type: `upstream-kernel-source` · Accessed: `2026-07-17`  
Scope: USB4 networking kernel configuration  
Source: <https://github.com/torvalds/linux/blob/af5e34a41cd607c00ef752e00331736570992354/drivers/net/thunderbolt/Kconfig>

Claims used in this snapshot:

- CONFIG_USB4_NET depends on USB4 and INET.
- The module is named thunderbolt_net and implements Apple ThunderboltIP interoperability.

## ROCMFPX-A5605

**ROCmFPX repository at a5605a7** — charlie12345  
Classification: `experimental-community` · Type: `community-experimental-source` · Accessed: `2026-07-17`  
Scope: Experimental ROCmFP3/4/6/8 llama.cpp fork  
Source: <https://github.com/charlie12345/ROCmFPX/tree/a5605a72768c6562241b248e268e33dc92787394>

Claims used in this snapshot:

- ROCmFPX is explicitly experimental.
- The fork contains CPU, HIP/ROCm, and Vulkan paths and reports local gfx1151 validation.

## ROCMFPX-BUILD-A5605

**ROCmFPX Strix Halo build script at a5605a7** — charlie12345  
Classification: `experimental-community` · Type: `community-experimental-build-recipe` · Accessed: `2026-07-17`  
Scope: Exact gfx1151 build flags  
Source: <https://github.com/charlie12345/ROCmFPX/blob/a5605a72768c6562241b248e268e33dc92787394/scripts/build-strix-rocmfp4-mtp.sh>

Claims used in this snapshot:

- Builds HIP and Vulkan for gfx1151, disables rocWMMA FlashAttention by default, and forces the fork MMQ path.

## ROCMFPX-DOCKER-A5605

**ROCmFPX Dockerfile at a5605a7** — charlie12345  
Classification: `experimental-community` · Type: `community-experimental-container` · Accessed: `2026-07-17`  
Scope: ROCm 7.2.1 Ubuntu 24.04 reproducible container baseline  
Source: <https://github.com/charlie12345/ROCmFPX/blob/a5605a72768c6562241b248e268e33dc92787394/.devops/strix-rocmfp4.Dockerfile>

Claims used in this snapshot:

- Pins the base stack to rocm/dev-ubuntu-24.04:7.2.1-complete and gfx1151.

## KYUZ0-TOOLBOX-A7C71E9

**AMD Strix Halo toolboxes at a7c71e9** — kyuz0  
Classification: `community` · Type: `maintained-community-profile` · Accessed: `2026-07-17`  
Scope: Current community host and container validation  
Source: <https://github.com/kyuz0/amd-strix-halo-toolboxes/tree/a7c71e91917234c44eb81b7fc64dc704db14879c>

Claims used in this snapshot:

- Reports Fedora 42/43, kernel 6.18.9, and linux-firmware 20260110 as a stable baseline.
- Provides ROCm 7.2.4, RADV, AMDVLK, and ROCmFP4 toolbox profiles.
- Recommends RADV for broad compatibility and warns about AMDVLK large allocations.

## ROCM-ISSUE-5724

**MES 0x83 firmware GPU hang and page fault on Strix Halo** — ROCm issue tracker  
Classification: `community` · Type: `community-reproducible-regression` · Accessed: `2026-07-17`  
Scope: late-2025 amdgpu firmware regression  
Source: <https://github.com/ROCm/ROCm/issues/5724>

Claims used in this snapshot:

- Documents GPU hangs and memory-access faults associated with the 20251125/MES 0x83 firmware era.

## ROCM-ISSUE-5590

**CWSR-related MES 0x80 hang on ROCm 7.1** — ROCm issue tracker  
Classification: `community` · Type: `community-reproducible-regression` · Accessed: `2026-07-17`  
Scope: ROCm 7.1 legacy stack  
Source: <https://github.com/ROCm/ROCm/issues/5590>

Claims used in this snapshot:

- Documents amdgpu.cwsr_enable=0 as a legacy workaround for a separate ROCm 7.1/MES 0x80 hang.

## ROCM-ISSUE-5824

**Strix Halo failures across old kernel, firmware, and ROCm combinations** — ROCm issue tracker  
Classification: `community` · Type: `community-reproducible-regression` · Accessed: `2026-07-17`  
Scope: known-bad combination evidence  
Source: <https://github.com/ROCm/ROCm/issues/5824>

Claims used in this snapshot:

- HSA_OVERRIDE_GFX_VERSION=11.0.0 or 11.0.3 did not repair the reported old-stack failures.

## ROCM-ISSUE-5745

**ROCm SMI and process D-state after HIP error** — ROCm issue tracker  
Classification: `community` · Type: `community-regression` · Accessed: `2026-07-17`  
Scope: post-fault recovery diagnostics  
Source: <https://github.com/ROCm/ROCm/issues/5745>

Claims used in this snapshot:

- Documents a post-error state where management tools or processes can block, motivating kernel-log collection before reset.

## ROCM-ISSUE-5444

**Historical Strix Halo visible-memory cap** — ROCm issue tracker  
Classification: `community` · Type: `community-historical-regression` · Accessed: `2026-07-17`  
Scope: kernel <=6.15 era memory visibility  
Source: <https://github.com/ROCm/ROCm/issues/5444>

Claims used in this snapshot:

- Documents the historical approximately 15.5 GiB visible-memory cap; the issue is closed and should not be confused with current TTM sizing.

## FEDORA-BUG-2420062

**Fedora linux-firmware regression affecting Strix Halo ROCm** — Fedora / Red Hat Bugzilla  
Classification: `community-distribution` · Type: `distribution-regression-record` · Accessed: `2026-07-17`  
Scope: linux-firmware 20251125 regression and 20260110 fix line  
Source: <https://bugzilla.redhat.com/show_bug.cgi?id=2420062>

Claims used in this snapshot:

- Tracks the bad 20251125 firmware and fixed 20260110 Fedora package line.

## LINUX-FW-REVERT-C092

**linux-firmware amdgpu firmware revert c092c748** — linux-firmware project  
Classification: `official-upstream` · Type: `upstream-firmware-commit` · Accessed: `2026-07-17`  
Scope: Firmware regression response  
Source: <https://git.kernel.org/pub/scm/linux/kernel/git/firmware/linux-firmware.git/commit/?id=c092c7487eb7c3d58697f490ff605bc38f4cc947>

Claims used in this snapshot:

- Upstream reverted the implicated amdgpu firmware update.

## THUNDERBOLT-IBVERBS-76BA39B

**thunderbolt-ibverbs at 76ba39b** — hellas-ai  
Classification: `experimental-community` · Type: `experimental-community-driver` · Accessed: `2026-07-17`  
Scope: InfiniBand verbs over USB4/Thunderbolt DMA rings  
Source: <https://github.com/hellas-ai/thunderbolt-ibverbs/tree/76ba39b630a70accb72f19388eefe48844b50eb8>

Claims used in this snapshot:

- The project explicitly labels itself buggy, insecure, and not for production.
- Requires Linux 6.14 or newer unless using its patched kernel.
- Provides DKMS, a libibverbs provider, Nix outputs, and container integration.

## STRIXHALO-WIKI-CLUSTERING

**Strix Halo community clustering notes** — Strix Halo Wiki  
Classification: `community` · Type: `community-guide` · Accessed: `2026-07-17`  
Scope: Thunderbolt networking throughput and clustering  
Source: <https://strixhalo.wiki/AI/Clustering>

Claims used in this snapshot:

- Reports system-specific thunderbolt-net throughput around 9 Gbit/s per link; treat as indicative rather than guaranteed.

## HIP-ENV-VARS

**HIP environment variables reference** — AMD HIP Documentation  
Classification: `official` · Type: `official-runtime-documentation` · Accessed: `2026-07-17`  
Scope: GPU visibility, debugging, and runtime behavior  
Source: <https://rocm.docs.amd.com/projects/HIP/en/latest/reference/env_variables.html>

Claims used in this snapshot:

- Documents HIP_VISIBLE_DEVICES and debugging-oriented HIP environment controls.

## AMD-PYTORCH-721-AOTRITON

**Install PyTorch for Ryzen on ROCm 7.2.1** — AMD ROCm Documentation  
Classification: `official` · Type: `official-application-install-guide` · Accessed: `2026-07-17`  
Scope: PyTorch runtime configuration for ROCm 7.2.1 on supported Ryzen systems  
Source: <https://rocm.docs.amd.com/projects/radeon-ryzen/en/docs-7.2.1/docs/install/installryz/native_linux/install-pytorch.html>

Claims used in this snapshot:

- The native PyTorch recipe exports TORCH_ROCM_AOTRITON_ENABLE_EXPERIMENTAL=1.
- The documented container line is ROCm 7.2.1 with Ubuntu 24.04, Python 3.12, and PyTorch 2.9.1.

## COMM-STRIX-GUIDE-2026

**Strix Halo Linux and ROCm community guide** — hogeheer499-commits  
Classification: `community` · Type: `community-guide` · Accessed: `2026-07-17`  
Scope: Measured troubleshooting notes for a kernel 6.19.x Strix Halo setup  
Source: <https://github.com/hogeheer499-commits/strix-halo-guide>

Claims used in this snapshot:

- Reports HSA_OVERRIDE_GFX_VERSION=11.5.1 together with HSA_ENABLE_SDMA=0 as a local workaround for a measured kernel 6.19.x setup.
- The workaround is system-specific evidence and is not an AMD support requirement or a general current-stack default.
