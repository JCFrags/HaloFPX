# Recipe: pinned ROCmFPX a5605 on gfx1151

**Classification:** experimental community  
**Pin:** `a5605a72768c6562241b248e268e33dc92787394`  
**Sources:** [ROCMFPX-A5605](../sources.md#rocmfpx-a5605), [ROCMFPX-BUILD-A5605](../sources.md#rocmfpx-build-a5605), [ROCMFPX-DOCKER-A5605](../sources.md#rocmfpx-docker-a5605)

## Container build

```bash
docker build \
  -f ../../containers/Dockerfile.rocmfpx \
  -t rocmfpx-strix:a5605-rocm721 \
  ../..
```

Run:

```bash
docker run --rm -it \
  --device=/dev/kfd --device=/dev/dri \
  --security-opt seccomp=unconfined \
  -v "$PWD/models:/models" \
  rocmfpx-strix:a5605-rocm721 \
  llama-cli --list-devices
```

## Native pinned build

```bash
ROCMFPX_COMMIT=a5605a72768c6562241b248e268e33dc92787394 \
  ../../scripts/build-rocmfpx.sh
```

## Quantize

```bash
./ROCmFPX/build-strix-rocmfp4/bin/llama-quantize \
  source-BF16.gguf output-ROCMFP4-FAST.gguf Q4_0_ROCMFP4_FAST
```

Hash both source and output and record the quantizer version.

## Run Vulkan and HIP separately

```bash
./ROCmFPX/build-strix-rocmfp4/bin/llama-cli \
  -m output-ROCMFP4-FAST.gguf -dev Vulkan0 -ngl 999 -fa on --jinja

HSA_OVERRIDE_GFX_VERSION=11.5.1 \
GGML_HIP_ENABLE_UNIFIED_MEMORY=1 \
./ROCmFPX/build-strix-rocmfp4/bin/llama-cli \
  -m output-ROCMFP4-FAST.gguf -dev ROCm0 -ngl 999 -fa on --jinja
```

## Quality gate

For every model family, compare BF16/F16 and ROCmFPX output on:

- perplexity or task-specific evaluation;
- JSON/tool-call validity;
- code compilation/tests;
- long-context retrieval;
- prompt processing and generation speed.

The fork’s agent/coherency presets are hypotheses about protected tensors, not a substitute for evaluation.

## Isolation rule

Do not export `HSA_OVERRIDE_GFX_VERSION` or `GGML_HIP_ENABLE_UNIFIED_MEMORY` globally. Put them in the ROCmFPX launch wrapper only.
