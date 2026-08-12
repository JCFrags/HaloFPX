#!/usr/bin/env bash
set -euo pipefail

root="/home/connorb/halofpx-lab/open-pin-01/candidate"
build="${root}/build-open-pin-01-rpc"
echo "timestamp=$(date --iso-8601=seconds)"
echo "host=$(hostname)"
df -BG /
git -C "${root}" rev-parse HEAD^{commit} HEAD^{tree}
git -C "${root}" status --porcelain=v1
grep -E 'MemAvailable|SwapTotal|SwapFree' /proc/meminfo

cmake -S "${root}" -B "${build}" \
  -DCMAKE_BUILD_TYPE=Release \
  -DGGML_HIP=ON \
  -DGGML_HIP_ROCWMMA_FATTN=OFF \
  -DGGML_HIP_FORCE_MMQ=ON \
  -DGGML_VULKAN=ON \
  -DGGML_CUDA=OFF \
  -DGGML_RPC=ON \
  -DCMAKE_HIP_ARCHITECTURES=gfx1151 \
  -DGPU_TARGETS=gfx1151 \
  -DCMAKE_HIP_FLAGS='-DGGML_HIP_ROCMFP4_FAST_FORCE_DEQUANT=1 -DGGML_HIP_ROCMFP4_FAST_QMMA=0 -DGGML_HIP_ROCMFP4_FAST_DECODE_QMMA=0' \
  -DLLAMA_BUILD_SERVER=ON \
  -DLLAMA_BUILD_WEBUI=OFF \
  -DLLAMA_USE_PREBUILT_WEBUI=OFF \
  -DLLAMA_BUILD_TESTS=OFF \
  -DGGML_BUILD_TESTS=OFF

time cmake --build "${build}" -j 16 --target rpc-server llama-server
sha256sum "${build}/bin/rpc-server" "${build}/bin/llama-server"
git -C "${root}" status --porcelain=v1
df -BG /
grep -E 'MemAvailable|SwapTotal|SwapFree' /proc/meminfo
