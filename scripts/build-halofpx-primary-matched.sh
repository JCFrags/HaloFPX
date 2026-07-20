#!/usr/bin/env bash
set -euo pipefail

if [[ $# -lt 3 || $# -gt 4 ]]; then
    echo "usage: $0 SOURCE_ROOT BUILD_ROOT EVIDENCE_ROOT [HIP_QUANT_KV_TILE_ON_OR_OFF]" >&2
    exit 2
fi

source_root=$1
build_root=$2
evidence_root=$3
hip_quant_kv_tile=${4:-OFF}
export HIPCXX=/opt/rocm/lib/llvm/bin/clang++
export HIP_PATH=/opt/rocm

if [[ ${hip_quant_kv_tile} != ON && ${hip_quant_kv_tile} != OFF ]]; then
    echo "HIP_QUANT_KV_TILE_ON_OR_OFF must be ON or OFF" >&2
    exit 2
fi

if [[ -e ${build_root}/CMakeCache.txt ]]; then
    echo "refusing to reuse configured build root: ${build_root}" >&2
    exit 2
fi

mkdir -p "${build_root}" "${evidence_root}"
date --iso-8601=ns >"${evidence_root}/started-at.txt"
uname -a >"${evidence_root}/uname.txt"
env | LC_ALL=C sort >"${evidence_root}/environment.txt"
cc --version >"${evidence_root}/cc-version.txt"
c++ --version >"${evidence_root}/cxx-version.txt"
cmake --version >"${evidence_root}/cmake-version.txt"
/opt/rocm/lib/llvm/bin/clang++ --version >"${evidence_root}/hip-compiler-version.txt"
sha256sum /opt/rocm/lib/llvm/bin/clang++ >"${evidence_root}/hip-compiler.sha256"

cmake -S "${source_root}" -B "${build_root}" -G Ninja \
    -DCMAKE_BUILD_TYPE=Release \
    -DCMAKE_HIP_ARCHITECTURES=gfx1151 \
    -DGPU_TARGETS=gfx1151 \
    -DGGML_HIP=ON \
    -DGGML_VULKAN=ON \
    -DGGML_RPC=ON \
    -DGGML_HIP_FORCE_MMQ=ON \
    -DGGML_HIP_NO_VMM=ON \
    -DGGML_HIP_QUANT_KV_FATTN_TILE="${hip_quant_kv_tile}" \
    -DGGML_VULKAN_FA_Q8_0_PREDEQUANT=OFF \
    -DLLAMA_BUILD_SERVER=ON \
    -DLLAMA_BUILD_TESTS=OFF \
    -DLLAMA_BUILD_WEBUI=OFF \
    >"${evidence_root}/configure.log" 2>&1

cmake --build "${build_root}" --target rpc-server llama-server --parallel 16 \
    >"${evidence_root}/build.log" 2>&1

sha256sum \
    "${build_root}/CMakeCache.txt" \
    "${build_root}/bin/rpc-server" \
    "${build_root}/bin/llama-server" \
    >"${evidence_root}/build.sha256"
grep -E '^(CMAKE_BUILD_TYPE|CMAKE_C_COMPILER:|CMAKE_CXX_COMPILER:|CMAKE_HIP_COMPILER:|CMAKE_HIP_ARCHITECTURES:|GPU_TARGETS:|GGML_HIP:|GGML_HIP_FORCE_MMQ:|GGML_HIP_NO_VMM:|GGML_HIP_QUANT_KV_FATTN_TILE:|GGML_RPC:|GGML_VULKAN:|GGML_VULKAN_FA_Q8_0_PREDEQUANT:|LLAMA_BUILD_SERVER:|LLAMA_BUILD_TESTS:|LLAMA_BUILD_WEBUI:)' \
    "${build_root}/CMakeCache.txt" | LC_ALL=C sort >"${evidence_root}/configuration.txt"
date --iso-8601=ns >"${evidence_root}/completed-at.txt"
