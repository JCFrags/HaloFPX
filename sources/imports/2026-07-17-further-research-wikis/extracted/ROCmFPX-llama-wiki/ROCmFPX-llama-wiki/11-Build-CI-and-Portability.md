# Build, CI, and portability

> **Audit snapshot:** fork `a5605a72768c6562241b248e268e33dc92787394`; upstream `86d86ed4396b4130922f7b9af26e3d9fc11a591b`; inventory date `2026-07-17`.  
> **Decision vocabulary:** **RETAIN** = capability/ABI must survive; **REFRESH** = preserve capability but re-port onto current upstream; **RETIRE** = remove the fork patch and use upstream or archive it. [S-FORK-HEAD](https://github.com/charlie12345/ROCmFPX/tree/a5605a72768c6562241b248e268e33dc92787394) [S-UP-HEAD](https://github.com/ggml-org/llama.cpp/tree/86d86ed4396b4130922f7b9af26e3d9fc11a591b)

## Build changes that carry product value

| Change | Path | Decision | Rationale | Primary sources |
|---|---|---|---|---|
| gfx1151 Strix profile | `CMakePresets.json`, `scripts/build-strix-rocmfp4-mtp.sh` | **RETAIN** | Encodes the target architecture and dual HIP/Vulkan build. | [S-CMAKE-PRESETS](https://github.com/charlie12345/ROCmFPX/blob/a5605a72768c6562241b248e268e33dc92787394/CMakePresets.json) [S-BUILD-STRIX](https://github.com/charlie12345/ROCmFPX/blob/a5605a72768c6562241b248e268e33dc92787394/scripts/build-strix-rocmfp4-mtp.sh) [S-AMD-STRIX](https://rocm.docs.amd.com/en/docs-7.2.0/how-to/system-optimization/strixhalo.html) |
| ROCmFPX tuning flags | `scripts/rocmfp4-decode-tune-flags.sh` | **RETAIN** | Provides reproducible parameterization for kernel sweeps. | [S-TUNE-FLAGS](https://github.com/charlie12345/ROCmFPX/blob/a5605a72768c6562241b248e268e33dc92787394/scripts/rocmfp4-decode-tune-flags.sh) [S-MMVQ](https://github.com/charlie12345/ROCmFPX/blob/a5605a72768c6562241b248e268e33dc92787394/ggml/src/ggml-cuda/mmvq.cu) |
| ROCmFPX CI | `.github/workflows/check-rocmfpx.yml` | **RETAIN** | Fork-only formats require reference and backend gates. | [S-ROCMFPX-CI](https://github.com/charlie12345/ROCmFPX/blob/a5605a72768c6562241b248e268e33dc92787394/.github/workflows/check-rocmfpx.yml) [S-C-848](https://github.com/charlie12345/ROCmFPX/commit/8488bfc69f716b5aa34bf75d1a72466ac75cc5da) |
| HIP quality/register gate | `.github/workflows/hip-quality-check.yml`, VGPR checker | **REFRESH** | Valuable compiler-resource guard, but workflow/toolchain must track upstream. | [S-HIP-QUALITY](https://github.com/charlie12345/ROCmFPX/blob/a5605a72768c6562241b248e268e33dc92787394/.github/workflows/hip-quality-check.yml) [S-PR27-FILES](https://github.com/charlie12345/ROCmFPX/pull/27/files) |
| Backend/model sweeps | `scripts/sweep-rocmfpx-*`, profile/preflight scripts | **RETAIN** | Encodes fork-specific correctness and performance acceptance. | [S-SWEEP-BACKEND](https://github.com/charlie12345/ROCmFPX/blob/a5605a72768c6562241b248e268e33dc92787394/scripts/sweep-rocmfpx-backend-ops.sh) [S-PR27-FILES](https://github.com/charlie12345/ROCmFPX/pull/27/files) |

## Hard-coded environment debt

The Strix build path contains local toolchain/header assumptions, including explicit compiler/header locations. These should become detected or parameterized inputs, with the resolved paths recorded in the build manifest. The gfx1151 target itself remains required. [S-BUILD-STRIX](https://github.com/charlie12345/ROCmFPX/blob/a5605a72768c6562241b248e268e33dc92787394/scripts/build-strix-rocmfp4-mtp.sh) [S-CMAKE-PRESETS](https://github.com/charlie12345/ROCmFPX/blob/a5605a72768c6562241b248e268e33dc92787394/CMakePresets.json)

## Generic build/CI patches to retire

PR #27 carries multiple generic fixes: disabling WebUI in unrelated backend jobs, provisioning embedded WebUI assets, restoring an XCFramework script, and importing dependency/backend snapshots. These are not ROCmFPX capabilities and should be replaced by current upstream build and workflow files. [S-C-063](https://github.com/charlie12345/ROCmFPX/commit/0631515b0bca7859387e3467a6f6ac6379622a02) [S-C-A8C](https://github.com/charlie12345/ROCmFPX/commit/a8c41d31423e3e0b6f2dc7138af1d4fba8edb295) [S-C-01D](https://github.com/charlie12345/ROCmFPX/commit/01d463bb23c3f290688c4529c13b3b467fa2f7dc) [S-C-807](https://github.com/charlie12345/ROCmFPX/commit/80732f992f7c75e9154cfe184041f1384c59a0fb) [S-UP-HEAD](https://github.com/ggml-org/llama.cpp/tree/86d86ed4396b4130922f7b9af26e3d9fc11a591b)

## CI layering after migration

1. **Format ABI:** compile CPU references and run golden byte vectors.
2. **Shared integration:** build CPU-only and run type-trait/quantizer/backend-op tests.
3. **HIP correctness:** compile and run each advertised ROCmFPX operation.
4. **Vulkan correctness:** run equivalent operation and packed-copy tests.
5. **Strix qualification:** run real-model coherency, MTP equivalence, and performance gates on gfx1151.
6. **Cross-platform smoke:** ensure unsupported backends reject/fallback safely without carrying fork snapshots.
7. **RPC:** same-build custom-type and target/draft multi-context tests.

The fork already contains examples for the first five layers; current upstream should supply the generic cross-platform matrix. [S-ROCMFP2-TEST](https://github.com/charlie12345/ROCmFPX/blob/a5605a72768c6562241b248e268e33dc92787394/ggml/rocmfpx/test_rocmfp2_reference.c) [S-BACKEND-TEST](https://github.com/charlie12345/ROCmFPX/blob/a5605a72768c6562241b248e268e33dc92787394/tests/test-backend-ops.cpp) [S-ROCMFPX-CI](https://github.com/charlie12345/ROCmFPX/blob/a5605a72768c6562241b248e268e33dc92787394/.github/workflows/check-rocmfpx.yml) [S-HIP-QUALITY](https://github.com/charlie12345/ROCmFPX/blob/a5605a72768c6562241b248e268e33dc92787394/.github/workflows/hip-quality-check.yml) [S-SWEEP-BACKEND](https://github.com/charlie12345/ROCmFPX/blob/a5605a72768c6562241b248e268e33dc92787394/scripts/sweep-rocmfpx-backend-ops.sh) [S-UP-HEAD](https://github.com/ggml-org/llama.cpp/tree/86d86ed4396b4130922f7b9af26e3d9fc11a591b)

## Reproducibility manifest

Every qualified binary/model result should record:

- fork extension commit and upstream commit;
- ROCm/HIP compiler and runtime versions;
- GPU target and reported device;
- Vulkan driver/device where used;
- CMake cache and tuning environment;
- model hash, quantization recipe, and GGUF type;
- context, batch, ubatch, offload, FlashAttention, speculative policy, prompt, and seed.

This is a recommended build-governance requirement derived from the fork’s hardware/compiler-sensitive tuning surface. [S-BUILD-STRIX](https://github.com/charlie12345/ROCmFPX/blob/a5605a72768c6562241b248e268e33dc92787394/scripts/build-strix-rocmfp4-mtp.sh) [S-TUNE-FLAGS](https://github.com/charlie12345/ROCmFPX/blob/a5605a72768c6562241b248e268e33dc92787394/scripts/rocmfp4-decode-tune-flags.sh) [S-README-FORK](https://github.com/charlie12345/ROCmFPX/blob/a5605a72768c6562241b248e268e33dc92787394/README.md) [S-HIP-QUALITY](https://github.com/charlie12345/ROCmFPX/blob/a5605a72768c6562241b248e268e33dc92787394/.github/workflows/hip-quality-check.yml)
