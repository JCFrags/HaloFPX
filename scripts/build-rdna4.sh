#!/usr/bin/env bash
# RDNA4 build — defaults to RX 9070-class gfx1201; RX 9060 is gfx1200.
set -euo pipefail
HIP_ARCH="${CMAKE_HIP_ARCHITECTURES:-gfx1201}"
exec env CMAKE_HIP_ARCHITECTURES="$HIP_ARCH" BUILD_DIR="${BUILD_DIR:-build-rdna4}" \
    "$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)/build-rocmfp4.sh" "$@"
