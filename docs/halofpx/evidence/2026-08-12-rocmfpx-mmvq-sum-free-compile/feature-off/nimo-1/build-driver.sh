#!/usr/bin/env bash

set -Eeuo pipefail

SOURCE_BUNDLE=/tmp/halofpx-sumfree-a3fd09e-20260812.bundle
SOURCE_ROOT=/tmp/halofpx-sumfree-off-a3fd09e
BUILD_ROOT=/tmp/halofpx-sumfree-off-build-a3fd09e
EVIDENCE_ROOT=/tmp/halofpx-sumfree-off-evidence-a3fd09e

mkdir -p "$EVIDENCE_ROOT"
git clone --branch codex/rocmfpx-mmvq-sum-free --single-branch \
    "$SOURCE_BUNDLE" "$SOURCE_ROOT" >"$EVIDENCE_ROOT/clone.log" 2>&1
git -C "$SOURCE_ROOT" checkout --detach a3fd09e0d522e20d0153bb2a07ddd09916249c8d

cmake -S "$SOURCE_ROOT" -B "$BUILD_ROOT" \
    -DCMAKE_BUILD_TYPE=Release \
    -DCMAKE_EXPORT_COMPILE_COMMANDS=ON \
    -DCMAKE_HIP_ARCHITECTURES=gfx1151 \
    -DBUILD_SHARED_LIBS=ON \
    -DGGML_BACKEND_DL=OFF \
    -DGGML_HIP=ON \
    -DGGML_HIP_ROCMFPX_MMVQ_SUM_FREE=OFF \
    -DGGML_RPC=OFF \
    -DLLAMA_BUILD_TESTS=ON \
    -DLLAMA_BUILD_TOOLS=OFF \
    -DLLAMA_BUILD_EXAMPLES=OFF \
    -DLLAMA_BUILD_SERVER=OFF \
    -DLLAMA_BUILD_WEBUI=OFF \
    2>&1 | tee "$EVIDENCE_ROOT/configure.log"

cmake --build "$BUILD_ROOT" \
    --target ggml-hip \
        test-halofpx-rocmfpx-mmvq-sum-free-off \
        test-halofpx-rocmfpx-mmvq-sum-free-on \
    --parallel 4 \
    2>&1 | tee "$EVIDENCE_ROOT/build.log"
