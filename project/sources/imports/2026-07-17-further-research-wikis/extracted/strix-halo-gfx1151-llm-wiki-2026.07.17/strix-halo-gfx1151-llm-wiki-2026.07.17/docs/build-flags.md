# Exact build flags

## Current pinned upstream llama.cpp HIP build

Pin: `b10064` / `86d86ed4396b4130922f7b9af26e3d9fc11a591b`.

```bash
git clone https://github.com/ggml-org/llama.cpp.git
cd llama.cpp
git checkout 86d86ed4396b4130922f7b9af26e3d9fc11a591b

export HIPCXX="$(hipconfig -l)/clang"
export HIP_PATH="$(hipconfig -R)"

cmake -S . -B build-hip -G Ninja   -DCMAKE_BUILD_TYPE=Release   -DGGML_HIP=ON   -DGGML_VULKAN=OFF   -DGGML_CUDA=OFF   -DCMAKE_HIP_ARCHITECTURES=gfx1151   -DGGML_HIP_ROCWMMA_FATTN=OFF   -DGGML_HIP_NO_VMM=ON   -DGGML_HIP_GRAPHS=ON   -DLLAMA_BUILD_TESTS=OFF   -DGGML_BUILD_TESTS=OFF

cmake --build build-hip --config Release -j"$(nproc)"
```

Rationale:

- `CMAKE_HIP_ARCHITECTURES=gfx1151` is the current direct CMake target control.
- `GGML_HIP_ROCWMMA_FATTN=OFF` matches current source default and avoids the reported long-context prefill regression until a local A/B proves otherwise.
- `GGML_HIP_NO_VMM=ON` and `GGML_HIP_GRAPHS=ON` preserve current upstream defaults explicitly so a future default change is visible in the recipe.
- The compiler and runtime are resolved from the same `hipconfig` root.

Sources: [LLAMA-HIP-CMAKE-86D86ED](sources.md#llama-hip-cmake-86d86ed), [LLAMA-GGML-CMAKE-86D86ED](sources.md#llama-ggml-cmake-86d86ed), [LLAMA-ISSUE-24437](sources.md#llama-issue-24437).

## Vulkan/RADV build

```bash
cmake -S . -B build-vulkan -G Ninja   -DCMAKE_BUILD_TYPE=Release   -DGGML_VULKAN=ON   -DGGML_HIP=OFF   -DGGML_CUDA=OFF   -DLLAMA_BUILD_TESTS=OFF   -DGGML_BUILD_TESTS=OFF
cmake --build build-vulkan --config Release -j"$(nproc)"
```

Use `vulkaninfo --summary` before building. The compiler does not need ROCm for a Vulkan-only build.

## Deliberate dual-backend build

A single binary can include HIP and Vulkan for matched A/B tests:

```bash
cmake -S . -B build-amd-dual -G Ninja   -DCMAKE_BUILD_TYPE=Release   -DGGML_HIP=ON   -DGGML_VULKAN=ON   -DGGML_CUDA=OFF   -DCMAKE_HIP_ARCHITECTURES=gfx1151   -DGGML_HIP_ROCWMMA_FATTN=OFF   -DGGML_HIP_NO_VMM=ON   -DGGML_HIP_GRAPHS=ON   -DLLAMA_BUILD_TESTS=OFF   -DGGML_BUILD_TESTS=OFF
cmake --build build-amd-dual -j"$(nproc)"
```

Use explicit runtime devices (`-dev ROCm0` and `-dev Vulkan0`) so automatic selection cannot invalidate a comparison.

## Target-variable precedence

| Control | Use |
|---|---|
| `CMAKE_HIP_ARCHITECTURES=gfx1151` | Preferred current CMake control |
| `GPU_TARGETS=gfx1151` | Compatibility input for projects that forward it |
| `AMDGPU_TARGETS=gfx1151` | Older compatibility alias; current llama.cpp forwards it when needed |

Do not set all three to different values. When switching ROCm versions or target variables, delete the build directory; CMake caches the compiler and architecture.

## ROCmFPX fork flags

The pinned fork’s own script uses:

```bash
cmake -S . -B build-strix-rocmfp4   -DCMAKE_BUILD_TYPE=Release   -DGGML_HIP=ON   -DGGML_HIP_ROCWMMA_FATTN=OFF   -DGGML_HIP_FORCE_MMQ=ON   -DGGML_VULKAN=ON   -DGGML_CUDA=OFF   -DCMAKE_HIP_ARCHITECTURES=gfx1151   -DGPU_TARGETS=gfx1151   -DLLAMA_BUILD_SERVER=ON   -DLLAMA_BUILD_WEBUI=OFF   -DLLAMA_USE_PREBUILT_WEBUI=OFF   -DLLAMA_BUILD_TESTS=ON   -DGGML_BUILD_TESTS=OFF
```

`GGML_HIP_FORCE_MMQ` is **fork-specific** in this context. Do not substitute it for upstream’s current option names. Source: [ROCMFPX-BUILD-A5605](sources.md#rocmfpx-build-a5605).

## CMake version policy

- CMake 3.21 is the effective minimum for current llama.cpp’s native HIP-language path and `CMAKE_HIP_ARCHITECTURES`.
- Ubuntu 24.04’s 3.28.x line is the conservative reproducible baseline used in these container recipes.
- CMake 4.3.4 is current upstream, but should be introduced as a separate matrix change and tested rather than silently replacing the baseline.

Sources: [CMAKE-HIP-ARCH](sources.md#cmake-hip-arch), [CMAKE-RELEASE-434](sources.md#cmake-release-434), [LLAMA-HIP-CMAKE-86D86ED](sources.md#llama-hip-cmake-86d86ed).
