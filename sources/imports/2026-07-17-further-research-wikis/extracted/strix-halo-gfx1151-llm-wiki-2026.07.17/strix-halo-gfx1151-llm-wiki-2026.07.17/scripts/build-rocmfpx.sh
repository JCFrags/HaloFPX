#!/usr/bin/env bash
set -euo pipefail

REPO="${ROCMFPX_REPO:-https://github.com/charlie12345/ROCmFPX.git}"
COMMIT="${ROCMFPX_COMMIT:-a5605a72768c6562241b248e268e33dc92787394}"
SRC_DIR="${SRC_DIR:-$PWD/ROCmFPX}"
JOBS="${JOBS:-$(nproc)}"

for cmd in git cmake; do command -v "$cmd" >/dev/null || { echo "Missing command: $cmd" >&2; exit 1; }; done
if [[ ! -d "$SRC_DIR/.git" ]]; then git clone "$REPO" "$SRC_DIR"; fi
if ! git -C "$SRC_DIR" cat-file -e "$COMMIT^{commit}" 2>/dev/null; then
    git -C "$SRC_DIR" fetch --depth=1 origin "$COMMIT"
fi
git -C "$SRC_DIR" checkout --detach "$COMMIT"

(
    cd "$SRC_DIR"
    export JOBS CMAKE_HIP_ARCHITECTURES="${CMAKE_HIP_ARCHITECTURES:-gfx1151}"
    export GGML_HIP_ROCWMMA_FATTN="${GGML_HIP_ROCWMMA_FATTN:-OFF}"
    scripts/build-strix-rocmfp4-mtp.sh
)

BUILD_DIR="${BUILD_DIR:-$SRC_DIR/build-strix-rocmfp4}"
{
    echo "rocmfpx_commit=$COMMIT"
    echo "repo=$REPO"
    echo "hip_arch=${CMAKE_HIP_ARCHITECTURES:-gfx1151}"
    echo "rocwmma_fattn=${GGML_HIP_ROCWMMA_FATTN:-OFF}"
    echo "built_at=$(date --iso-8601=seconds 2>/dev/null || date)"
} >"$BUILD_DIR/STRIX_BUILD_PROVENANCE.txt"
find "$BUILD_DIR/bin" -maxdepth 1 -type f -executable -print0 2>/dev/null | sort -z | xargs -0 -r sha256sum >"$BUILD_DIR/bin/SHA256SUMS"
echo "Built ROCmFPX: $BUILD_DIR"
