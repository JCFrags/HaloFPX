#!/usr/bin/env bash
set -euo pipefail

LLAMA_REPO="${LLAMA_REPO:-https://github.com/ggml-org/llama.cpp.git}"
LLAMA_COMMIT="${LLAMA_COMMIT:-86d86ed4396b4130922f7b9af26e3d9fc11a591b}"
SRC_DIR="${SRC_DIR:-$PWD/llama.cpp}"
BUILD_DIR="${BUILD_DIR:-$SRC_DIR/build-hip}"
JOBS="${JOBS:-$(nproc)}"
HIP_ARCH="${CMAKE_HIP_ARCHITECTURES:-gfx1151}"

version_ge() { [[ "$(printf '%s\n%s\n' "$2" "$1" | sort -V | head -n1)" == "$2" ]]; }
for cmd in git cmake ninja; do command -v "$cmd" >/dev/null || { echo "Missing command: $cmd" >&2; exit 1; }; done
CMAKE_VERSION="$(cmake --version | awk 'NR==1{print $3}')"
version_ge "$CMAKE_VERSION" 3.21 || { echo "CMake >=3.21 required; found $CMAKE_VERSION" >&2; exit 1; }

if [[ ! -d "$SRC_DIR/.git" ]]; then
    git clone "$LLAMA_REPO" "$SRC_DIR"
fi
if ! git -C "$SRC_DIR" cat-file -e "$LLAMA_COMMIT^{commit}" 2>/dev/null; then
    git -C "$SRC_DIR" fetch --depth=1 origin "$LLAMA_COMMIT"
fi
git -C "$SRC_DIR" checkout --detach "$LLAMA_COMMIT"

if command -v hipconfig >/dev/null 2>&1; then
    ROCM_PATH="${ROCM_PATH:-$(hipconfig -R)}"
    HIPCXX="${HIPCXX:-$(hipconfig -l)/clang}"
    HIP_PATH="${HIP_PATH:-$(hipconfig -R)}"
else
    ROCM_PATH="${ROCM_PATH:-/opt/rocm}"
    HIPCXX="${HIPCXX:-$ROCM_PATH/llvm/bin/clang}"
    HIP_PATH="${HIP_PATH:-$ROCM_PATH}"
fi
export ROCM_PATH HIPCXX HIP_PATH

[[ -x "$HIPCXX" ]] || { echo "HIP compiler not executable: $HIPCXX" >&2; exit 1; }
if [[ -n "${HSA_OVERRIDE_GFX_VERSION:-}" ]]; then
    echo "Warning: HSA_OVERRIDE_GFX_VERSION=$HSA_OVERRIDE_GFX_VERSION is active during build" >&2
fi

cmake -S "$SRC_DIR" -B "$BUILD_DIR" -G Ninja \
  -DCMAKE_BUILD_TYPE=Release \
  -DCMAKE_PREFIX_PATH="$ROCM_PATH" \
  -DGGML_HIP=ON \
  -DGGML_VULKAN=OFF \
  -DGGML_CUDA=OFF \
  -DCMAKE_HIP_ARCHITECTURES="$HIP_ARCH" \
  -DGGML_HIP_ROCWMMA_FATTN=OFF \
  -DGGML_HIP_NO_VMM=ON \
  -DGGML_HIP_GRAPHS=ON \
  -DLLAMA_BUILD_TESTS=OFF \
  -DGGML_BUILD_TESTS=OFF
cmake --build "$BUILD_DIR" --config Release -j "$JOBS"

{
    echo "llama_commit=$LLAMA_COMMIT"
    echo "source=$LLAMA_REPO"
    echo "rocm_path=$ROCM_PATH"
    echo "hipcxx=$HIPCXX"
    echo "hip_path=$HIP_PATH"
    echo "hip_arch=$HIP_ARCH"
    echo "cmake_version=$CMAKE_VERSION"
    echo "built_at=$(date --iso-8601=seconds 2>/dev/null || date)"
    "$HIPCXX" --version | sed 's/^/compiler=/'
} >"$BUILD_DIR/STRIX_BUILD_PROVENANCE.txt"
cmake -LAH -N "$BUILD_DIR" >"$BUILD_DIR/cmake-cache-options.txt" 2>&1 || true
find "$BUILD_DIR/bin" -maxdepth 1 -type f -executable -print0 2>/dev/null | sort -z | xargs -0 -r sha256sum >"$BUILD_DIR/bin/SHA256SUMS"

echo "Built pinned HIP tree: $BUILD_DIR"
