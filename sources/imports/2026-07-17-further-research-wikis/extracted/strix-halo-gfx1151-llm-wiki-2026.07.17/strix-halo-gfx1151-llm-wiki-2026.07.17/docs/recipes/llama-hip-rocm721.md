# Recipe: pinned llama.cpp b10064 with ROCm 7.2.1

**Classification:** AMD target-stable ROCm release plus official upstream source  
**Host floor:** Ubuntu HWE `>=6.17.0-19.19~24.04.2`, OEM `>=6.14.0-1018`, or kernel `>=6.18.4` elsewhere  
**Sources:** [AMD-RDNA35](../sources.md#amd-rdna35), [LLAMA-RELEASE-B10064](../sources.md#llama-release-b10064)

## Prefer a clean container for the compiler

```bash
docker build \
  -f ../../containers/Dockerfile.llama-rocm721 \
  -t llama-strix:rocm721-b10064 \
  ../..
```

Run on a qualified host:

```bash
docker run --rm -it \
  --device=/dev/kfd \
  --device=/dev/dri \
  --security-opt seccomp=unconfined \
  -v "$PWD/models:/models:ro" \
  llama-strix:rocm721-b10064 \
  llama-cli --list-devices
```

Record and replace the base tag with an OCI digest for a production lock.

## Native source build

```bash
export ROCM_PATH=/opt/rocm
export HIPCXX="$(hipconfig -l)/clang"
export HIP_PATH="$(hipconfig -R)"
unset HSA_OVERRIDE_GFX_VERSION

LLAMA_COMMIT=86d86ed4396b4130922f7b9af26e3d9fc11a591b \
  ../../scripts/build-llama-hip.sh
```

Expanded configuration:

```bash
cmake -S llama.cpp -B llama.cpp/build-hip -G Ninja \
  -DCMAKE_BUILD_TYPE=Release \
  -DGGML_HIP=ON \
  -DGGML_VULKAN=OFF \
  -DGGML_CUDA=OFF \
  -DCMAKE_HIP_ARCHITECTURES=gfx1151 \
  -DGGML_HIP_ROCWMMA_FATTN=OFF \
  -DGGML_HIP_NO_VMM=ON \
  -DGGML_HIP_GRAPHS=ON \
  -DLLAMA_BUILD_TESTS=OFF \
  -DGGML_BUILD_TESTS=OFF
```

## Prebuilt binary alternative

```bash
../../scripts/download-llama-b10064-rocm72.sh
./downloads/llama-b10064-rocm72/llama-cli --version
```

The archive is checked against:

```text
42a00452f42b04598d32db66c5249b3e8855cd99bf9448e22fd2a738aaa89c82
```

Do not use this ROCm 7.2 binary as a shortcut inside an arbitrary ROCm 7.14 runtime. Rebuild from source for the 7.14 lane.

## Runtime and benchmark

```bash
HIP_VISIBLE_DEVICES=0 \
./llama.cpp/build-hip/bin/llama-cli \
  -m MODEL.gguf -ngl 999 -fa 1 -c 8192 -p 'gfx1151 smoke' -n 64

./llama.cpp/build-hip/bin/llama-bench \
  -m MODEL.gguf -p 512 -n 128 -ngl 999 -fa 1
```

Keep rocWMMA FlashAttention OFF until a matched local build proves it improves the actual context and model mix.
