# Environment variables and kernel parameters

Machine-readable table: [`environment-variables.csv`](../data/environment-variables.csv).

| Variable/parameter | Class | Recommended use | Example | Risk | Evidence |
| --- | --- | --- | --- | --- | --- |
| `ROCM_PATH` | official | Set to the exact unpacked SDK root for a tarball install. | `export ROCM_PATH=/opt/rocm-7.14.0` | A stale value can mix release lines. | [AMD-INSTALL-714](sources.md#amd-install-714), [LLAMA-HIP-CMAKE-86D86ED](sources.md#llama-hip-cmake-86d86ed) |
| `PATH` | official | Prepend or append one ROCm SDK bin directory consistently. | `export PATH="$ROCM_PATH/bin:$PATH"` | Multiple hipconfig binaries can select the wrong SDK. | [AMD-INSTALL-714](sources.md#amd-install-714) |
| `LD_LIBRARY_PATH` | official-for-tarball | Use only for nonstandard/tarball installs; prefer a container or ldconfig file for deployment. | `export LD_LIBRARY_PATH="$ROCM_PATH/lib${LD_LIBRARY_PATH:+:$LD_LIBRARY_PATH}"` | Can silently load libraries from the wrong release line. | [AMD-INSTALL-714](sources.md#amd-install-714) |
| `HIPCXX` | upstream-recipe | Resolve from the active ROCm SDK. | `export HIPCXX="$(hipconfig -l)/clang"` | Do not point it to a system Clang from another generation. | [LLAMA-ROCM-DOCKER-86D86ED](sources.md#llama-rocm-docker-86d86ed), [LLAMA-HIP-CMAKE-86D86ED](sources.md#llama-hip-cmake-86d86ed) |
| `HIP_PATH` | upstream-recipe | Resolve from active hipconfig. | `export HIP_PATH="$(hipconfig -R)"` | Mismatch with HIPCXX creates a mixed toolchain. | [LLAMA-ROCM-DOCKER-86D86ED](sources.md#llama-rocm-docker-86d86ed) |
| `CMAKE_HIP_ARCHITECTURES` | official-upstream | Set gfx1151 explicitly for a single-target Strix Halo build. | `cmake ... -DCMAKE_HIP_ARCHITECTURES=gfx1151` | Omitting it can produce a larger or wrong-target build; requires CMake >=3.21. | [CMAKE-HIP-ARCH](sources.md#cmake-hip-arch), [LLAMA-HIP-CMAKE-86D86ED](sources.md#llama-hip-cmake-86d86ed) |
| `GPU_TARGETS` | compatibility-alias | Use only for projects/ROCm packages that consume it; direct CMAKE_HIP_ARCHITECTURES is clearer in current llama.cpp. | `cmake ... -DGPU_TARGETS=gfx1151` | Project-specific behavior. | [LLAMA-HIP-CMAKE-86D86ED](sources.md#llama-hip-cmake-86d86ed), [ROCMFPX-BUILD-A5605](sources.md#rocmfpx-build-a5605) |
| `AMDGPU_TARGETS` | legacy-compatibility-alias | Prefer CMAKE_HIP_ARCHITECTURES for current CMake projects. | `cmake ... -DAMDGPU_TARGETS=gfx1151` | Can be ignored or translated depending on project version. | [LLAMA-HIP-CMAKE-86D86ED](sources.md#llama-hip-cmake-86d86ed) |
| `CMAKE_HIP_COMPILER_ROCM_ROOT` | advanced-upstream | Set only when a nonstandard SDK layout cannot be auto-discovered. | `cmake ... -DCMAKE_HIP_COMPILER_ROCM_ROOT="$ROCM_PATH"` | Can hide an inconsistent install if used reflexively. | [CMAKE-HIP-ARCH](sources.md#cmake-hip-arch), [LLAMA-HIP-CMAKE-86D86ED](sources.md#llama-hip-cmake-86d86ed) |
| `HIP_VISIBLE_DEVICES` | official | Use for deterministic single-GPU selection. | `export HIP_VISIBLE_DEVICES=0` | Device numbering can differ from other APIs. | [HIP-ENV-VARS](sources.md#hip-env-vars) |
| `ROCR_VISIBLE_DEVICES` | official | Use when HSA-level filtering is required; avoid contradictory visibility variables. | `export ROCR_VISIBLE_DEVICES=0` | Contradictory HIP and ROCR filters can confuse diagnostics. | [HIP-ENV-VARS](sources.md#hip-env-vars) |
| `HIP_LAUNCH_BLOCKING` | diagnostic | Set to 1 only to localize an asynchronous fault. | `HIP_LAUNCH_BLOCKING=1 ./build/bin/llama-bench ...` | Severely changes timing and performance. | [HIP-ENV-VARS](sources.md#hip-env-vars) |
| `TORCH_ROCM_AOTRITON_ENABLE_EXPERIMENTAL` | official-pytorch-recipe | Set to 1 only for the AMD ROCm 7.2.1 PyTorch recipe that documents it; it is not a llama.cpp, HIP, or system-wide requirement. | `export TORCH_ROCM_AOTRITON_ENABLE_EXPERIMENTAL=1` | Experimental application behavior; irrelevant to llama.cpp and may change across PyTorch/ROCm release lines. | [AMD-PYTORCH-721-AOTRITON](sources.md#amd-pytorch-721-aotriton) |
| `HSA_OVERRIDE_GFX_VERSION` | project-specific-or-unsupported-workaround | Leave unset on a native current gfx1151 stack. The ROCmFPX fork documents 11.5.1 for its own HIP path. | `export HSA_OVERRIDE_GFX_VERSION=11.5.1  # ROCmFPX recipe only` | 11.0.0/11.0.3 spoofing did not fix old-stack failures and can select incompatible code paths. | [ROCMFPX-A5605](sources.md#rocmfpx-a5605), [ROCM-ISSUE-5824](sources.md#rocm-issue-5824) |
| `GGML_HIP_ENABLE_UNIFIED_MEMORY` | ROCmFPX-project-specific | Use only with the pinned ROCmFPX fork recipe that documents it. | `export GGML_HIP_ENABLE_UNIFIED_MEMORY=1` | Not an upstream llama.cpp compatibility requirement. | [ROCMFPX-A5605](sources.md#rocmfpx-a5605) |
| `VK_DRIVER_FILES` | official-upstream | Set only to force a verified ICD manifest during controlled A/B tests. | `export VK_DRIVER_FILES=/usr/share/vulkan/icd.d/radeon_icd.x86_64.json` | A wrong path makes Vulkan devices disappear; ignored in elevated contexts. | [VULKAN-LOADER-ICD](sources.md#vulkan-loader-icd) |
| `VK_ICD_FILENAMES` | deprecated-upstream | Use VK_DRIVER_FILES instead for new recipes. | `unset VK_ICD_FILENAMES` | Stale shell configuration can force AMDVLK or a nonexistent driver unexpectedly. | [VULKAN-LOADER-ICD](sources.md#vulkan-loader-icd) |
| `VK_LOADER_DEBUG` | official-upstream | Use all for a one-shot loader trace. | `VK_LOADER_DEBUG=all vulkaninfo --summary 2>vulkan-loader.log` | Verbose; may expose filesystem paths in logs. | [VULKAN-LOADER-ICD](sources.md#vulkan-loader-icd) |
| `GGML_CUDA_DISABLE_GRAPHS` | related-community-diagnostic | Not a gfx1151 default; use only when investigating graph-related faults and record the result. | `GGML_CUDA_DISABLE_GRAPHS=1 ./llama-cli ...` | May reduce performance and is sourced from a related RDNA4 report. | [LLAMA-ISSUE-24961](sources.md#llama-issue-24961) |
| `HSA_ENABLE_SDMA` | system-specific-community-workaround | Leave enabled on a validated current stack. Set to 0 only for a bounded A/B test or the cited kernel 6.19.x community setup; remove it after diagnosis unless independently reproduced. | `HSA_ENABLE_SDMA=0 ./reproducer  # diagnostic or cited 6.19.x setup only` | Changes transfer paths and performance; can conceal rather than repair a kernel, firmware, or runtime mismatch. | [COMM-STRIX-GUIDE-2026](sources.md#comm-strix-guide-2026) |
| `amdgpu.cwsr_enable` | legacy-workaround | Leave default on current supported stacks; use =0 only to contain the documented legacy ROCm 7.1/MES issue while migrating. | `amdgpu.cwsr_enable=0` | Disables a kernel feature and can mask an obsolete stack. | [ROCM-ISSUE-5590](sources.md#rocm-issue-5590), [ROCM-ISSUE-5724](sources.md#rocm-issue-5724) |
| `amd_iommu` | community-performance-tuning | Do not disable IOMMU by default when USB4, virtualization, device isolation, or RDMA security matters; benchmark in your threat model. | `amd_iommu=off  # community performance profile only` | Disabling IOMMU weakens DMA isolation and may conflict with USB4/RDMA security requirements. | [KYUZ0-TOOLBOX-A7C71E9](sources.md#kyuz0-toolbox-a7c71e9), [THUNDERBOLT-IBVERBS-76BA39B](sources.md#thunderbolt-ibverbs-76ba39b) |

## Minimal clean-shell profiles

### ROCm 7.14 tarball

```bash
export ROCM_PATH=/opt/rocm-7.14.0
export PATH="$ROCM_PATH/bin:$PATH"
export LD_LIBRARY_PATH="$ROCM_PATH/lib${LD_LIBRARY_PATH:+:$LD_LIBRARY_PATH}"
export HIPCXX="$ROCM_PATH/llvm/bin/clang"
export HIP_PATH="$ROCM_PATH"
unset HSA_OVERRIDE_GFX_VERSION VK_ICD_FILENAMES VK_DRIVER_FILES
```

### ROCm installed under `/opt/rocm`

```bash
export ROCM_PATH=/opt/rocm
export HIPCXX="$(hipconfig -l)/clang"
export HIP_PATH="$(hipconfig -R)"
unset HSA_OVERRIDE_GFX_VERSION
```

### RADV A/B test

```bash
unset VK_ICD_FILENAMES
export VK_DRIVER_FILES=/usr/share/vulkan/icd.d/radeon_icd.x86_64.json
VK_LOADER_DEBUG=all vulkaninfo --summary 2>vulkan-loader.log
```

Verify the manifest path before exporting it. Do not place `VK_DRIVER_FILES` in a global shell profile unless the machine intentionally has one Vulkan ICD.

## Variables intentionally not made defaults

- `HSA_OVERRIDE_GFX_VERSION`: native current gfx1151 stacks should identify themselves correctly. The `11.5.1` setting is scoped to the pinned ROCmFPX recipe; `11.0.x` spoofing is explicitly rejected as a generic fix.
- `HSA_ENABLE_SDMA=0`: diagnostic only; it changes transfer paths.
- `HIP_LAUNCH_BLOCKING=1`: diagnostic only; it changes timing.
- `GGML_CUDA_DISABLE_GRAPHS=1`: related architecture diagnostic, not a gfx1151 baseline.
- `amdgpu.cwsr_enable=0`: legacy ROCm 7.1 containment, not a current requirement.
- `amd_iommu=off`: community performance tuning with security and device-isolation consequences.
