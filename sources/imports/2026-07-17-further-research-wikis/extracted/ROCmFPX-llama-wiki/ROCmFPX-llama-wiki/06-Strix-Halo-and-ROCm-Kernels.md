# Strix Halo and ROCm kernels

> **Audit snapshot:** fork `a5605a72768c6562241b248e268e33dc92787394`; upstream `86d86ed4396b4130922f7b9af26e3d9fc11a591b`; inventory date `2026-07-17`.  
> **Decision vocabulary:** **RETAIN** = capability/ABI must survive; **REFRESH** = preserve capability but re-port onto current upstream; **RETIRE** = remove the fork patch and use upstream or archive it. [S-FORK-HEAD](https://github.com/charlie12345/ROCmFPX/tree/a5605a72768c6562241b248e268e33dc92787394) [S-UP-HEAD](https://github.com/ggml-org/llama.cpp/tree/86d86ed4396b4130922f7b9af26e3d9fc11a591b)

## Hardware target

AMD identifies Strix Halo as Ryzen AI MAX/MAX+ with integrated RDNA 3.5 graphics, target `gfx1151`, and unified memory mapped through GPUVM/GTT rather than a discrete VRAM pool. AMD’s system guide also documents kernel and memory-configuration requirements for reliable compute use. [S-AMD-STRIX](https://rocm.docs.amd.com/en/docs-7.2.0/how-to/system-optimization/strixhalo.html)

The fork’s Strix build path explicitly targets `gfx1151`, enables HIP and Vulkan, forces the quantized MMQ path, builds the server/tests, and disables CUDA. [S-BUILD-STRIX](https://github.com/charlie12345/ROCmFPX/blob/a5605a72768c6562241b248e268e33dc92787394/scripts/build-strix-rocmfp4-mtp.sh) [S-CMAKE-PRESETS](https://github.com/charlie12345/ROCmFPX/blob/a5605a72768c6562241b248e268e33dc92787394/CMakePresets.json)

## Build and tuning surface

| Knob or path | Fork behavior | Assessment | Primary sources |
|---|---|---|---|
| `AMDGPU_TARGETS` / `CMAKE_HIP_ARCHITECTURES` | `gfx1151` | **RETAIN** as an explicit profile. | [S-BUILD-STRIX](https://github.com/charlie12345/ROCmFPX/blob/a5605a72768c6562241b248e268e33dc92787394/scripts/build-strix-rocmfp4-mtp.sh) [S-CMAKE-PRESETS](https://github.com/charlie12345/ROCmFPX/blob/a5605a72768c6562241b248e268e33dc92787394/CMakePresets.json) [S-AMD-STRIX](https://rocm.docs.amd.com/en/docs-7.2.0/how-to/system-optimization/strixhalo.html) |
| HIP + Vulkan in one build | Enables both backends for comparative/runtime selection. | **RETAIN**, but make dependencies discoverable. | [S-BUILD-STRIX](https://github.com/charlie12345/ROCmFPX/blob/a5605a72768c6562241b248e268e33dc92787394/scripts/build-strix-rocmfp4-mtp.sh) |
| RDNA3.5 warp count | ROCmFPX MMVQ code contains RDNA3.5-specific warp defaults. | **REFRESH** and rebenchmark per compiler/ROCm release. | [S-TUNE-FLAGS](https://github.com/charlie12345/ROCmFPX/blob/a5605a72768c6562241b248e268e33dc92787394/scripts/rocmfp4-decode-tune-flags.sh) [S-MMVQ](https://github.com/charlie12345/ROCmFPX/blob/a5605a72768c6562241b248e268e33dc92787394/ggml/src/ggml-cuda/mmvq.cu) |
| MMID maximum batch | Fork tuning controls batched indirect-MoE dispatch thresholds. | **REFRESH** with model-shape test matrix. | [S-TUNE-FLAGS](https://github.com/charlie12345/ROCmFPX/blob/a5605a72768c6562241b248e268e33dc92787394/scripts/rocmfp4-decode-tune-flags.sh) [S-MMVQ](https://github.com/charlie12345/ROCmFPX/blob/a5605a72768c6562241b248e268e33dc92787394/ggml/src/ggml-cuda/mmvq.cu) |
| Rows per block / wide dispatch | Fork exposes wide row and MoE-row controls. | **REFRESH** with occupancy/register gates. | [S-TUNE-FLAGS](https://github.com/charlie12345/ROCmFPX/blob/a5605a72768c6562241b248e268e33dc92787394/scripts/rocmfp4-decode-tune-flags.sh) [S-MMVQ](https://github.com/charlie12345/ROCmFPX/blob/a5605a72768c6562241b248e268e33dc92787394/ggml/src/ggml-cuda/mmvq.cu) [S-HIP-QUALITY](https://github.com/charlie12345/ROCmFPX/blob/a5605a72768c6562241b248e268e33dc92787394/.github/workflows/hip-quality-check.yml) |
| Local rocWMMA headers | Optional experimental path in the build script. | **RETIRE as default**; keep opt-in only. | [S-BUILD-STRIX](https://github.com/charlie12345/ROCmFPX/blob/a5605a72768c6562241b248e268e33dc92787394/scripts/build-strix-rocmfp4-mtp.sh) [S-AMD-ROCWMMA](https://rocm.docs.amd.com/projects/rocWMMA/en/develop/api-reference/api-reference-guide.html#supported-gpu-architectures) |

Current official rocWMMA documentation lists RDNA targets `gfx1100`, `gfx1101`, `gfx1102`, `gfx1200`, and `gfx1201`, but not `gfx1151`. That does not prove rocWMMA cannot compile on Strix Halo; it does mean the fork should not treat gfx1151 rocWMMA as an officially listed default path. [S-AMD-ROCWMMA](https://rocm.docs.amd.com/projects/rocWMMA/en/develop/api-reference/api-reference-guide.html#supported-gpu-architectures)

## HIP/ROCm kernel inventory

| Function | Primary fork paths | ROCmFPX-specific content | Decision | Primary sources |
|---|---|---|---|---|
| Device dequantization/codebook | `ggml/rocmfpx/rocmfpx_hip_codebook.cuh`, `ggml/src/ggml-cuda/dequantize.cuh` | Packed-code decode and scale application for custom formats. | **RETAIN/REFRESH** | [S-HIP-CODEBOOK](https://github.com/charlie12345/ROCmFPX/blob/a5605a72768c6562241b248e268e33dc92787394/ggml/rocmfpx/rocmfpx_hip_codebook.cuh) [S-DEQUANT](https://github.com/charlie12345/ROCmFPX/blob/a5605a72768c6562241b248e268e33dc92787394/ggml/src/ggml-cuda/dequantize.cuh) |
| Quantized vector dot | `ggml/src/ggml-cuda/vecdotq.cuh` | ROCmFPX dot-product specializations. | **REFRESH** | [S-VECDOT](https://github.com/charlie12345/ROCmFPX/blob/a5605a72768c6562241b248e268e33dc92787394/ggml/src/ggml-cuda/vecdotq.cuh) |
| Decode GEMV/MMVQ | `ggml/src/ggml-cuda/mmvq.cu` | Custom format routing, Q2 path, RDNA3.5 warp/batch/MoE tuning. | **REFRESH** | [S-MMVQ](https://github.com/charlie12345/ROCmFPX/blob/a5605a72768c6562241b248e268e33dc92787394/ggml/src/ggml-cuda/mmvq.cu) |
| Prefill/batched MMQ | `ggml/src/ggml-cuda/mmq.cu`, `mmq.cuh` | Quantized tile loads and custom format template instantiations. | **REFRESH** | [S-MMQ](https://github.com/charlie12345/ROCmFPX/blob/a5605a72768c6562241b248e268e33dc92787394/ggml/src/ggml-cuda/mmq.cu) [S-MMQ-H](https://github.com/charlie12345/ROCmFPX/blob/a5605a72768c6562241b248e268e33dc92787394/ggml/src/ggml-cuda/mmq.cuh) [S-C-62F](https://github.com/charlie12345/ROCmFPX/commit/62f7508b12c6b8510fd7a77dfc5d9519fa026d82) |
| Copy/convert/rows | `convert.cu`, `cpy-utils.cuh`, `getrows.cu`, `set-rows.cu` | Custom packed types and operation support. | **REFRESH** | [S-PR32-FILES](https://github.com/charlie12345/ROCmFPX/pull/32/files) [S-CUDA-DISPATCH](https://github.com/charlie12345/ROCmFPX/blob/a5605a72768c6562241b248e268e33dc92787394/ggml/src/ggml-cuda/ggml-cuda.cu) |
| Backend capability/dispatch | `ggml/src/ggml-cuda/ggml-cuda.cu` | Type support and operation routing. | **REFRESH** | [S-CUDA-DISPATCH](https://github.com/charlie12345/ROCmFPX/blob/a5605a72768c6562241b248e268e33dc92787394/ggml/src/ggml-cuda/ggml-cuda.cu) |
| Reduction/softmax/MoE correctness | `common.cuh`, `softmax.cu`, `topk-moe.cu`, related files | ROCm 7.2 exact negative-infinity behavior. | **REFRESH** | [S-C-DF3](https://github.com/charlie12345/ROCmFPX/commit/df3b8a0efa4dbdfcc2e29dac367c69eed310ed24) |

## Coherency repair that must survive

Commit `62f7508…` corrects ROCm MMQ block-stride calculations and Vulkan handling of packed FP6 blocks when asynchronous loader chunks split a 26-byte block. The latter is a format/storage invariant, not a transient performance tweak, and requires direct unaligned copy tests after any rebase. [S-C-62F](https://github.com/charlie12345/ROCmFPX/commit/62f7508b12c6b8510fd7a77dfc5d9519fa026d82) [S-ROCMFPX-H](https://github.com/charlie12345/ROCmFPX/blob/a5605a72768c6562241b248e268e33dc92787394/ggml/rocmfpx/rocmfpx.h) [S-VULKAN](https://github.com/charlie12345/ROCmFPX/blob/a5605a72768c6562241b248e268e33dc92787394/ggml/src/ggml-vulkan/ggml-vulkan.cpp)

## Vulkan on Strix

The fork implements custom Vulkan shader types, dequantization, matrix-vector/matrix-matrix paths, and packed FP6 host/device conversion. The project README reports Vulkan as the faster decode backend in some local Strix measurements; that evidence is repository-reported and should be rerun under the target driver, compiler, model, and prompt. [S-VULKAN](https://github.com/charlie12345/ROCmFPX/blob/a5605a72768c6562241b248e268e33dc92787394/ggml/src/ggml-vulkan/ggml-vulkan.cpp) [S-VULKAN-TYPES](https://github.com/charlie12345/ROCmFPX/blob/a5605a72768c6562241b248e268e33dc92787394/ggml/src/ggml-vulkan/vulkan-shaders/types.glsl) [S-VULKAN-FP6](https://github.com/charlie12345/ROCmFPX/blob/a5605a72768c6562241b248e268e33dc92787394/ggml/src/ggml-vulkan/vulkan-shaders/dequant_rocmfpx_fp6.comp) [S-README-FORK](https://github.com/charlie12345/ROCmFPX/blob/a5605a72768c6562241b248e268e33dc92787394/README.md)

## Acceptance criteria for refreshed kernels

A refreshed backend is acceptable only when it passes:

1. byte-exact CPU-reference vectors;
2. backend operation matrices for every advertised operation;
3. unaligned host/device copy tests for transformed storage;
4. real-model coherency probes on HIP and Vulkan;
5. Strix performance gates with compiler, ROCm, driver, model, context, batch, and prompt recorded.

The fork already contains the constituent reference tests, backend tests, workflows, and sweep scripts; the migration should make them a formal release contract. [S-ROCMFP2-TEST](https://github.com/charlie12345/ROCmFPX/blob/a5605a72768c6562241b248e268e33dc92787394/ggml/rocmfpx/test_rocmfp2_reference.c) [S-BACKEND-TEST](https://github.com/charlie12345/ROCmFPX/blob/a5605a72768c6562241b248e268e33dc92787394/tests/test-backend-ops.cpp) [S-ROCMFPX-CI](https://github.com/charlie12345/ROCmFPX/blob/a5605a72768c6562241b248e268e33dc92787394/.github/workflows/check-rocmfpx.yml) [S-HIP-QUALITY](https://github.com/charlie12345/ROCmFPX/blob/a5605a72768c6562241b248e268e33dc92787394/.github/workflows/hip-quality-check.yml) [S-SWEEP-BACKEND](https://github.com/charlie12345/ROCmFPX/blob/a5605a72768c6562241b248e268e33dc92787394/scripts/sweep-rocmfpx-backend-ops.sh)
