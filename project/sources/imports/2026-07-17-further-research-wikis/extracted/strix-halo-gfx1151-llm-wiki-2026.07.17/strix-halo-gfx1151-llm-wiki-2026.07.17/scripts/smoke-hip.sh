#!/usr/bin/env bash
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
SRC="${HIP_SMOKE_SOURCE:-$ROOT/scripts/smoke-hip.cpp}"
BUILD_DIR="${HIP_SMOKE_BUILD_DIR:-$ROOT/.smoke-build}"
ARCH="${CMAKE_HIP_ARCHITECTURES:-gfx1151}"
mkdir -p "$BUILD_DIR"

if command -v hipcc >/dev/null 2>&1; then
    HIPCC="$(command -v hipcc)"
elif [[ -n "${ROCM_PATH:-}" && -x "$ROCM_PATH/bin/hipcc" ]]; then
    HIPCC="$ROCM_PATH/bin/hipcc"
else
    echo "hipcc not found; set ROCM_PATH or PATH" >&2
    exit 1
fi

if [[ -n "${HSA_OVERRIDE_GFX_VERSION:-}" ]]; then
    echo "Warning: HSA_OVERRIDE_GFX_VERSION=$HSA_OVERRIDE_GFX_VERSION is active" >&2
fi
"$HIPCC" -O2 --offload-arch="$ARCH" "$SRC" -o "$BUILD_DIR/smoke-hip"
sha256sum "$BUILD_DIR/smoke-hip"
"$BUILD_DIR/smoke-hip"
