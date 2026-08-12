#!/usr/bin/env bash
# RDNA3 build — defaults to gfx1100; set the exact target for other cards.
set -euo pipefail
HIP_ARCH="${CMAKE_HIP_ARCHITECTURES:-gfx1100}"
exec env CMAKE_HIP_ARCHITECTURES="$HIP_ARCH" BUILD_DIR="${BUILD_DIR:-build-rdna3}" \
    "$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)/build-rocmfp4.sh" "$@"
