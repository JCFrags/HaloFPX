# ROCmFPX on Strix Halo

## Classification

ROCmFPX is an **experimental community fork**, not an AMD-supported ROCm component and not an upstream ggml-org llama.cpp feature set in the form documented here. Pin commit:

```text
a5605a72768c6562241b248e268e33dc92787394
```

The fork describes AMD-focused ROCmFP3, ROCmFP4, ROCmFP6, and ROCmFP8 GGUF weight formats with CPU reference paths and accelerated HIP/ROCm and Vulkan kernels. Sources: [ROCMFPX-A5605](sources.md#rocmfpx-a5605).

## Reproducible baseline

The project Dockerfile pins:

```text
Ubuntu 24.04
rocm/dev-ubuntu-24.04:7.2.1-complete
gfx1151
```

Source: [ROCMFPX-DOCKER-A5605](sources.md#rocmfpx-docker-a5605).

Build:

```bash
git clone https://github.com/charlie12345/ROCmFPX.git
cd ROCmFPX
git checkout a5605a72768c6562241b248e268e33dc92787394
env JOBS=16 scripts/build-strix-rocmfp4-mtp.sh
```

The script builds HIP and Vulkan, targets `gfx1151`, keeps rocWMMA FlashAttention OFF, and enables the fork’s forced MMQ path. Exact expanded flags are in [Build flags](build-flags.md).

## Runtime

Vulkan:

```bash
build-strix-rocmfp4/bin/llama-cli   -m MODEL-ROCMFP4.gguf -dev Vulkan0 -ngl 999 -fa on --jinja
```

HIP/ROCm, using the fork-documented environment:

```bash
export HSA_OVERRIDE_GFX_VERSION=11.5.1
export GGML_HIP_ENABLE_UNIFIED_MEMORY=1
build-strix-rocmfp4/bin/llama-cli   -m MODEL-ROCMFP4.gguf -dev ROCm0 -ngl 999 -fa on --jinja
```

Those variables are scoped to this pinned fork recipe. Native upstream llama.cpp on a current gfx1151 ROCm stack should normally run without `HSA_OVERRIDE_GFX_VERSION`.

## Quantization examples

```bash
build-strix-rocmfp4/bin/llama-quantize   model-BF16.gguf model-ROCMFP4-FAST.gguf Q4_0_ROCMFP4_FAST
```

Other documented families include `Q3_0_ROCMFPX`, `Q4_0_ROCMFP4`, `Q6_0_ROCMFPX`, `Q8_0_ROCMFPX`, and agent/coherency variants. Compare every output to the BF16/F16 source for task-specific quality.

## Tuning profiles

The pinned tree exposes profile names including:

```text
rocmfpx-strix-moe-rpb1..4
rocmfpx-strix-nwarps1,2,4
rocmfpx-strix-rpb2
rocmfpx-strix-mmid1..4
rocmfpx-strix-vdr2,8
```

Set `ROCMFPX_DECODE_TUNE=PROFILE` only for controlled benchmark runs. The stable profile emits no extra compile definitions.

## Unsupported combinations

- Do not assume ROCm 7.14 works with this commit merely because Core SDK supports gfx1151; the fork’s reproducible container is ROCm 7.2.1.
- Do not copy fork-only `GGML_HIP_FORCE_MMQ` or unified-memory variables into upstream llama.cpp builds.
- Do not use the fork’s local rocWMMA header path unless the headers are deliberately pinned and the feature is A/B tested.
- Do not treat maintainer benchmark numbers as model-independent.
