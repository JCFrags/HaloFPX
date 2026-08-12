# Versioned compatibility matrix

Matrix version: **2026.07.17** · Target: **Strix Halo / gfx1151** · Rows: **21**

Machine-readable forms:

- [`compatibility-matrix-2026.07.17.csv`](../data/compatibility-matrix-2026.07.17.csv)
- [`compatibility-matrix-2026.07.17.json`](../data/compatibility-matrix-2026.07.17.json)
- [`compatibility-matrix-2026.07.17.yaml`](../data/compatibility-matrix-2026.07.17.yaml)
- [`component-versions-2026.07.17.csv`](../data/component-versions-2026.07.17.csv)

> [!CAUTION]
> Rows are scoped profiles, not a Cartesian product. Do not combine a kernel from one row, firmware from another, and a compiler from a third without revalidation.

## Component-level status

| Component | Official/current | Official target floor/scope | Community-validated | Known bad or risky | Evidence |
| --- | --- | --- | --- | --- | --- |
| Linux kernel | Ubuntu 26.04 kernel 7.0 or Ubuntu 24.04.4 HWE 6.17 for ROCm 7.14 | HWE >=6.17.0-19.19~24.04.2; OEM >=6.14.0-1018; other distros >=6.18.4 | Fedora 43 kernel 6.18.9 | Below distro-specific fixed minimum | [AMD-CORE-714](sources.md#amd-core-714), [AMD-RDNA35](sources.md#amd-rdna35), [KYUZ0-TOOLBOX-A7C71E9](sources.md#kyuz0-toolbox-a7c71e9) |
| amdgpu firmware | Use supported OS inbox/distribution pair | No single cross-distro package version stated; fixed firmware required | Fedora linux-firmware 20260110 | linux-firmware 20251125 / MES 0x83 era | [AMD-CORE-714](sources.md#amd-core-714), [FEDORA-BUG-2420062](sources.md#fedora-bug-2420062), [ROCM-ISSUE-5724](sources.md#rocm-issue-5724) |
| ROCm | 7.14 Core SDK | 7.2.1-7.2.3 stable in RDNA3.5 prebuilt table on qualifying kernels | 7.2.4 toolbox on Fedora 42/43 | 7.1.x and 6.4.x unsupported in current RDNA3.5 table | [AMD-CORE-714](sources.md#amd-core-714), [AMD-RDNA35](sources.md#amd-rdna35), [KYUZ0-TOOLBOX-A7C71E9](sources.md#kyuz0-toolbox-a7c71e9) |
| HIP | 7.14 | llama.cpp source requires HIP >=6.1; use supported ROCm line | 7.2.1 and 7.2.4 paths | Mixed HIP runtime/compiler generations | [AMD-CORE-714](sources.md#amd-core-714), [AMD-CORE-724](sources.md#amd-core-724), [LLAMA-HIP-CMAKE-86D86ED](sources.md#llama-hip-cmake-86d86ed) |
| LLVM/Clang | 23.0.0 with ROCm 7.14 | Use compiler bundled with selected ROCm SDK | 22.0.0.26084 with ROCm 7.2.4 | System Clang or LLVM from another ROCm generation | [AMD-CORE-714](sources.md#amd-core-714), [AMD-CORE-724](sources.md#amd-core-724) |
| Mesa/RADV | Mesa 26.1.5 upstream | No Mesa project hardware-support certification claim captured | Mesa 26.0.2/26.0.3 on gfx1151 llama.cpp reports; RADV recommended by toolbox | Upgrade regressions must be benchmark-gated | [MESA-2615](sources.md#mesa-2615), [LLAMA-ISSUE-22375](sources.md#llama-issue-22375), [LLAMA-ISSUE-24438](sources.md#llama-issue-24438), [KYUZ0-TOOLBOX-A7C71E9](sources.md#kyuz0-toolbox-a7c71e9) |
| AMDVLK | Driver release independent of this matrix | Not asserted | Works for selected models | Reported ~2 GiB single-buffer allocation limit for large models | [LLAMA-ISSUE-15054](sources.md#llama-issue-15054), [KYUZ0-TOOLBOX-A7C71E9](sources.md#kyuz0-toolbox-a7c71e9) |
| CMake | 4.3.4 | 3.21 for native HIP language and CMAKE_HIP_ARCHITECTURES | Ubuntu 24.04 3.28.x conservative baseline | Latest CMake not assumed validated with every old ROCm package | [CMAKE-RELEASE-434](sources.md#cmake-release-434), [CMAKE-HIP-ARCH](sources.md#cmake-hip-arch), [LLAMA-HIP-CMAKE-86D86ED](sources.md#llama-hip-cmake-86d86ed) |
| llama.cpp | b10064 / 86d86ed on 2026-07-17 | Upstream binary is ROCm 7.2; host support external | HIP and Vulkan on gfx1151; workload-dependent winner | rocWMMA FA long-context regression; commit-to-commit RADV pp regression | [LLAMA-RELEASE-B10064](sources.md#llama-release-b10064), [LLAMA-ISSUE-24437](sources.md#llama-issue-24437), [LLAMA-ISSUE-24438](sources.md#llama-issue-24438), [LLAMA-ISSUE-22375](sources.md#llama-issue-22375) |
| ROCmFPX | Not AMD/upstream llama official | N/A | a5605a7 with ROCm 7.2.1 container, HIP and Vulkan | Experimental APIs, formats, tuning and fork-only flags | [ROCMFPX-A5605](sources.md#rocmfpx-a5605), [ROCMFPX-BUILD-A5605](sources.md#rocmfpx-build-a5605), [ROCMFPX-DOCKER-A5605](sources.md#rocmfpx-docker-a5605) |
| USB4 IP networking | CONFIG_USB4_NET / thunderbolt_net | Kernel component independent of gfx ISA | Linux-to-Linux clustering; ~9 Gbit/s/link reported on specific systems | Cable/topology/authorization/MTU variability | [LINUX-THUNDERBOLT-DOC](sources.md#linux-thunderbolt-doc), [LINUX-USB4-NET-KCONFIG](sources.md#linux-usb4-net-kconfig), [STRIXHALO-WIKI-CLUSTERING](sources.md#strixhalo-wiki-clustering) |
| USB4 RDMA verbs | No upstream production support claim | N/A | thunderbolt-ibverbs 76ba39b, Linux >=6.14 | Explicitly buggy, insecure, research-only | [THUNDERBOLT-IBVERBS-76BA39B](sources.md#thunderbolt-ibverbs-76ba39b) |

## Profile matrix

| Profile | Class | Status | OS | Kernel | ROCm | Mesa/Vulkan | llama.cpp | Primary caveat |
| --- | --- | --- | --- | --- | --- | --- | --- | --- |
| OFF-CORE-714-U2604 | official-supported | supported-core-sdk | Ubuntu 26.04 | 7.0 GA | 7.14.0 | Host-dependent; not part of HIP support claim | Source build candidate; not AMD application-certified by this matrix | Do not infer PyTorch/llama.cpp/ROCmFPX application support from Core SDK support alone. |
| OFF-CORE-714-U2404-HWE | official-supported | supported-core-sdk | Ubuntu 24.04.4 | 6.17 HWE | 7.14.0 | Host-dependent; not part of HIP support claim | Source build candidate or upstream ROCm 7.2 binary only; do not mix binary runtime lines | Do not run the b10064 ROCm 7.2 prebuilt against a 7.14-only userspace unless all dynamic dependencies resolve to its intended line. |
| OFF-RDNA35-ROCM721-723 | official-supported | stable-on-qualified-kernel | Ubuntu 24.04 or another supported distribution | Ubuntu HWE >=6.17.0-19.19~24.04.2; Ubuntu OEM >=6.14.0-1018; others >=6.18.4 | 7.2.1, 7.2.2, or 7.2.3 | Independent path; RADV community validation below | Current source can be built; upstream b10064 ROCm 7.2 binary is compatible in release scope | rocWMMA FlashAttention can regress long-context prefill; firmware and kernel are independent gates. |
| OFF-RYZEN-72-U24043 | official-supported | supported-or-preliminary-by-application | Ubuntu 24.04.3 via documented 24.04.2 installer path | Must also satisfy the RDNA 3.5 fixed-minimum kernel rule | 7.2 application line | Not part of AMD HIP app claim | Listed application/use-case; exact release pin remains upstream-controlled | “Preliminary” OS support and “production” framework support are distinct labels. |
| UPSTREAM-LLAMA-B10064-ROCM72 | official-upstream-current | released-binary | Ubuntu-compatible Linux userspace | Host must independently meet gfx1151 requirements | ROCm 7.2 runtime line | Separate Vulkan assets/build path | b10064 / 86d86ed4396b4130922f7b9af26e3d9fc11a591b | Binary release scope does not guarantee host kernel, firmware, or every model path. |
| SYNTH-LLAMA-B10064-ROCM714 | reproducible-candidate | build-candidate-not-hardware-validated-here | Ubuntu 24.04.4 HWE 6.17 or Ubuntu 26.04 kernel 7.0 | Per ROCm 7.14 official matrix | 7.14.0 gfx1151 TheRock tarball | Optional; disable for HIP-only build | b10064 / 86d86ed4396b4130922f7b9af26e3d9fc11a591b | This is a reasoned pinned build combination, not a published AMD llama.cpp application-validation row. |
| COMM-F43-K6189-FW20260110-ROCM724 | community-validated | maintained-community-known-good | Fedora 42/43 | 6.18.9-200.fc43.x86_64 reported stable baseline | 7.2.4 toolbox | RADV preferred; AMDVLK available with allocation caveat | Toolbox tracks upstream master; pin a commit for reproducibility | ROCm 7.2.4 is a general official release and community-validated here, but is not named in AMD’s gfx1151-specific 7.2.1–7.2.3 row. |
| COMM-RADV-MESA2602-2603 | community-validated | recommended-community-vulkan-path | Modern Linux distribution | Qualifying gfx1151 kernel; reports include 6.19.4 | Not required for Vulkan | Mesa RADV 26.0.2 or 26.0.3 community-tested | Current source; pin b10064 for reproducibility | Prompt-processing regressions have occurred between llama.cpp builds; --no-direct-io/--no-mmap can isolate model-load failures. |
| UPSTREAM-MESA-2615-CANDIDATE | official-upstream-current | current-release-not-target-certified | Linux | Use a qualifying gfx1151 kernel | Not required for Vulkan | Mesa 26.1.5 / RADV | b10064 pinned build candidate | “Latest” is not equivalent to “validated on gfx1151.” Run the included Vulkan smoke and llama-bench gate. |
| RISK-AMDVLK-LARGE-ALLOC | community-reported-risk | conditionally-usable | Linux | Qualifying gfx1151 kernel | Not required | AMDVLK | Vulkan backend | Approximately 2 GiB single-buffer allocation failures have been reported; prefer RADV for large models. |
| EXP-ROCMFPX-A5605-ROCM721 | experimental-community | experimental-validated-by-maintainer | Ubuntu 24.04 container or compatible host | Qualifying gfx1151 host kernel | 7.2.1 complete container baseline | mesa-vulkan-drivers in runtime image; maintainer reports Vulkan and HIP tests | Fork commit a5605a72768c6562241b248e268e33dc92787394 | Fork-specific flags and environment variables must not be generalized to upstream llama.cpp. APIs and formats may change. |
| OFF-USB4-IP-THUNDERBOLT-NET | official-upstream-supported | supported-kernel-component | Linux with USB4, INET, and CONFIG_USB4_NET | Mainline kernel with thunderbolt_net; use gfx1151-qualified kernel for the overall host | Independent | Independent | Use TCP RPC over thunderbolt0 as normal IP networking | Interface naming and authorization depend on firmware, topology, and distribution policy; benchmark each cable/topology. |
| EXP-USB4-RDMA-76BA39B | experimental-community | research-only-not-production | Linux; packages for Debian/Ubuntu, Fedora, Arch, and Nix | >=6.14 or project-patched linux-thunderbolt kernel | Can be exposed into ROCm/vLLM/llama containers | Independent | Potential RDMA-aware distributed transports; project integration required | Project explicitly states buggy, insecure, and not for production. |
| HIST-AMD-LLAMA-B7146-ROCM711 | official-historical | historical-do-not-deploy-new | Ubuntu 24.04 | Historical installer prerequisites | 7.1.1 | Not material | b7146 AMD bundle | AMD’s current RDNA3.5 table marks ROCm 7.1.x unsupported on qualifying kernels; CWSR/MES hangs were reported. |
| BAD-FIRMWARE-20251125-MES083 | unsupported-known-bad | avoid | Affected Linux distributions | A new kernel does not neutralize the bad firmware by itself | Failures reported across 7.1 and later test paths | GPU stability risk can affect both compute and graphics paths | llama-bench segfaults/hangs reported in distribution tracking | GPU page faults, memory-access faults, hangs, and process D-state behavior. |
| BAD-KERNEL-BELOW-RDNA35-FIX | unsupported-known-bad | avoid-for-current-rocm | Linux | Below Ubuntu HWE 6.17.0-19, OEM 6.14.0-1018, or 6.18.4 elsewhere | Current prebuilt support is experimental or unsupported below thresholds | May show separate issues | Compute instability or memory limitations possible | Queue, memory, and visibility failures; historical approximately 15.5 GiB cap on much older kernels. |
| BAD-ROCM71-CURRENT-KERNEL | unsupported-known-bad | unsupported | Linux | Even a qualifying current kernel | 7.1.x | Independent | Historical package/builds only | CWSR/MES hangs and historical application regressions. |
| BAD-HSA-OVERRIDE-GFX110X | unsupported-workaround | do-not-use-as-fix | Linux | Any | Old or mismatched stacks | Not applicable | Not a supported upstream requirement on native current gfx1151 stacks | The reported 11.0.x overrides did not repair the underlying old-stack failures. |
| RISK-ROCWMMA-FA-GFX1151 | community-reported-regression | disable-by-default | Linux | Qualifying gfx1151 kernel | Reported on 7.2.1 | Not applicable | Current default is GGML_HIP_ROCWMMA_FATTN=OFF | Reported long-context prefill loss up to roughly 41% when enabled; retest before enabling. |
| RISK-MIXED-ROCM-LLVM-GENERATIONS | unsupported-by-scope | avoid | Linux | Any supported host | Do not mix 7.14 libraries with 7.2 compiler packages or vice versa | Independent | Clean CMake cache when switching SDK roots | Mismatched CMake packages, bitcode libraries, runtime DSOs, and compiler resource directories can cause configure, link, or runtime failures. |
| HIST-VISIBLE-MEMORY-15GB | historical-regression | closed-historical | Linux | <=6.15 era | 6.4.1 through early 7.0-era reports | Not primary cause | Large models could not use full unified memory | Approximately 15.5 GiB visible-memory cap; issue closed. |

## Recommended operational profiles

### A. Current official Core SDK profile

Use `OFF-CORE-714-U2604` or `OFF-CORE-714-U2404-HWE` when the objective is current ROCm SDK development. Keep ROCm 7.14, HIP 7.14, and LLVM 23 in one SDK root. Build current llama.cpp from source as a **candidate** and run the acceptance suite before deployment.

### B. Conservative ROCm 7.2 llama.cpp profile

Use `OFF-RDNA35-ROCM721-723` with a qualifying kernel and fixed firmware. Pin llama.cpp b10064. Explicitly compile `gfx1151`, leave rocWMMA FlashAttention OFF, and use the bundled ROCm LLVM 22 compiler.

### C. Maintained community profile

Use `COMM-F43-K6189-FW20260110-ROCM724` for the maintained Fedora toolbox path. Preserve its classification: ROCm 7.2.4 is official generally and community-validated on Strix Halo, not yet named by the captured AMD gfx1151-specific prebuilt row.

### D. Vulkan-first profile

Use `COMM-RADV-MESA2602-2603` for community-tested RADV versions or `UPSTREAM-MESA-2615-CANDIDATE` to test the latest upstream source. Pin both Mesa and llama.cpp and gate upgrades with `llama-bench` prompt-processing and token-generation thresholds.

### E. Experimental profiles

`EXP-ROCMFPX-A5605-ROCM721` and `EXP-USB4-RDMA-76BA39B` are deliberately separated from supported lanes. Their fork-only flags, kernel modules, and security risks are not inherited by upstream or official profiles.
