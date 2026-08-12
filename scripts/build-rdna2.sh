#!/usr/bin/env bash
# RDNA2 build — defaults to gfx1030 (RX 6800/6900); exact-target override supported
set -euo pipefail
HIP_ARCH="${CMAKE_HIP_ARCHITECTURES:-gfx1030}"
exec env CMAKE_HIP_ARCHITECTURES="$HIP_ARCH" BUILD_DIR="${BUILD_DIR:-build-rdna2}" \
    "$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)/build-rocmfp4.sh" "$@"
