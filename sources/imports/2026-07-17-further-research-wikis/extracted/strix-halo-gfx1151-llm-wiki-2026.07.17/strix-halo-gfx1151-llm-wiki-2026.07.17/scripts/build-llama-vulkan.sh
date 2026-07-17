#!/usr/bin/env bash
set -euo pipefail

LLAMA_REPO="${LLAMA_REPO:-https://github.com/ggml-org/llama.cpp.git}"
LLAMA_COMMIT="${LLAMA_COMMIT:-86d86ed4396b4130922f7b9af26e3d9fc11a591b}"
SRC_DIR="${SRC_DIR:-$PWD/llama.cpp}"
BUILD_DIR="${BUILD_DIR:-$SRC_DIR/build-vulkan}"
JOBS="${JOBS:-$(nproc)}"

for cmd in git cmake ninja; do command -v "$cmd" >/dev/null || { echo "Missing command: $cmd" >&2; exit 1; }; done
if command -v vulkaninfo >/dev/null 2>&1; then
    vulkaninfo --summary || { echo "vulkaninfo failed; verify ICD selection" >&2; exit 1; }
else
    echo "Warning: vulkaninfo not found; build can continue but runtime is unverified" >&2
fi

if [[ ! -d "$SRC_DIR/.git" ]]; then git clone "$LLAMA_REPO" "$SRC_DIR"; fi
if ! git -C "$SRC_DIR" cat-file -e "$LLAMA_COMMIT^{commit}" 2>/dev/null; then
    git -C "$SRC_DIR" fetch --depth=1 origin "$LLAMA_COMMIT"
fi
git -C "$SRC_DIR" checkout --detach "$LLAMA_COMMIT"

cmake -S "$SRC_DIR" -B "$BUILD_DIR" -G Ninja \
  -DCMAKE_BUILD_TYPE=Release \
  -DGGML_VULKAN=ON \
  -DGGML_HIP=OFF \
  -DGGML_CUDA=OFF \
  -DLLAMA_BUILD_TESTS=OFF \
  -DGGML_BUILD_TESTS=OFF
cmake --build "$BUILD_DIR" --config Release -j "$JOBS"

{
    echo "llama_commit=$LLAMA_COMMIT"
    echo "source=$LLAMA_REPO"
    echo "cmake_version=$(cmake --version | awk 'NR==1{print $3}')"
    echo "vk_driver_files=${VK_DRIVER_FILES:-unset}"
    echo "built_at=$(date --iso-8601=seconds 2>/dev/null || date)"
    vulkaninfo --summary 2>/dev/null || true
} >"$BUILD_DIR/STRIX_BUILD_PROVENANCE.txt"
cmake -LAH -N "$BUILD_DIR" >"$BUILD_DIR/cmake-cache-options.txt" 2>&1 || true
find "$BUILD_DIR/bin" -maxdepth 1 -type f -executable -print0 2>/dev/null | sort -z | xargs -0 -r sha256sum >"$BUILD_DIR/bin/SHA256SUMS"

echo "Built pinned Vulkan tree: $BUILD_DIR"
