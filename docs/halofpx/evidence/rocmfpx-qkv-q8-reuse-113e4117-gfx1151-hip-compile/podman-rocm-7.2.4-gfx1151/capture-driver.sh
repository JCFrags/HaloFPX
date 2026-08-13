#!/usr/bin/env bash
set -Eeuo pipefail

repo=/workspace/src
work=/workspace/work
evidence=/workspace/evidence
expected_head=113e411706e02704cf1c6c01d8973acbe0cab5b9

mkdir -p "${work}" "${evidence}"

{
    printf 'captured_utc='
    date --utc --iso-8601=seconds
    cat /etc/os-release
    uname -a
    printf 'dev_kfd='
    if [[ -e /dev/kfd ]]; then echo PRESENT; else echo ABSENT; fi
    printf 'dev_dri='
    if [[ -e /dev/dri ]]; then echo PRESENT; else echo ABSENT; fi
} 2>&1 | tee "${evidence}/container-environment.txt"

apt-get update 2>&1 | tee "${evidence}/apt-update-output.txt"
DEBIAN_FRONTEND=noninteractive apt-get install -y --no-install-recommends \
    build-essential cmake git hipblas-dev ninja-build python3 rocblas-dev \
    2>&1 | tee "${evidence}/apt-install-output.txt"
rm -rf /var/lib/apt/lists/*
git config --global --add safe.directory "${repo}"

{
    /opt/rocm/bin/hipconfig --full
    /opt/rocm/lib/llvm/bin/clang++ --version
    cmake --version
    ninja --version
    python3 --version
    dpkg-query -W -f='${Package}\t${Version}\n' \
        build-essential cmake git hipblas-dev ninja-build python3 rocblas-dev
} 2>&1 | tee "${evidence}/toolchain.txt"

actual_head=$(git -C "${repo}" rev-parse HEAD)
test "${actual_head}" = "${expected_head}"
{
    printf 'source_head=%s\n' "${actual_head}"
    git -C "${repo}" status --porcelain=v2 --branch
    git -C "${repo}" ls-tree -r HEAD -- \
        .github/workflows/halofpx-ci.yml \
        ggml/CMakeLists.txt \
        ggml/src/ggml-cuda/common.cuh \
        ggml/src/ggml-cuda/ggml-cuda.cu \
        ggml/src/ggml-cuda/mmq.cu \
        ggml/src/ggml-cuda/mmq.cuh \
        ggml/src/ggml-cuda/rocmfpx-qkv-q8-reuse.h \
        ggml/src/ggml-hip/CMakeLists.txt \
        tests/CMakeLists.txt \
        tests/test-backend-ops.cpp \
        tests/test-halofpx-rocmfpx-qkv-q8-reuse.cpp \
        tests/test-halofpx-rocmfpx-qkv-q8-reuse-source-contract.cmake
} > "${evidence}/source-state-and-blobs.txt"

for mode in off on; do
    if [[ "${mode}" = on ]]; then
        option=ON
    else
        option=OFF
    fi
    build_dir="${work}/build-${mode}"
    test ! -e "${build_dir}"

    cmake -S "${repo}" -B "${build_dir}" -G Ninja \
        -DCMAKE_BUILD_TYPE=Release \
        -DBUILD_SHARED_LIBS=ON \
        -DCMAKE_EXPORT_COMPILE_COMMANDS=ON \
        -DCMAKE_HIP_COMPILER=/opt/rocm/lib/llvm/bin/clang++ \
        -DCMAKE_HIP_ARCHITECTURES=gfx1151 \
        -DGPU_TARGETS=gfx1151 \
        -DGGML_CUDA=OFF \
        -DGGML_HIP=ON \
        -DGGML_HIP_ROCMFPX_QKV_Q8_REUSE="${option}" \
        -DGGML_HIP_ROCMFPX_FFN_Q8_REUSE=OFF \
        -DGGML_NATIVE=OFF \
        -DGGML_RPC=OFF \
        -DGGML_VULKAN=OFF \
        -DLLAMA_BUILD_TESTS=OFF \
        -DLLAMA_BUILD_TOOLS=OFF \
        -DLLAMA_BUILD_EXAMPLES=OFF \
        -DLLAMA_BUILD_SERVER=OFF \
        -DLLAMA_BUILD_WEBUI=OFF \
        -DLLAMA_OPENSSL=OFF \
        -DLLAMA_CURL=OFF \
        2>&1 | tee "${evidence}/configure-${mode}-output.txt"

    cmake --build "${build_dir}" --target ggml-hip --parallel 1 \
        2>&1 | tee "${evidence}/build-${mode}-output.txt"

    MODE="${mode}" BUILD_DIR="${build_dir}" python3 - <<'PY' \
        2>&1 | tee "${evidence}/compile-contract-${mode}-output.txt"
import json
import os
from pathlib import Path

mode = os.environ["MODE"]
build_dir = Path(os.environ["BUILD_DIR"])
entries = json.loads((build_dir / "compile_commands.json").read_text())
for suffix in ("/ggml/src/ggml-cuda/ggml-cuda.cu", "/ggml/src/ggml-cuda/mmq.cu"):
    matches = [entry for entry in entries if entry["file"].replace("\\", "/").endswith(suffix)]
    if len(matches) != 1:
        raise SystemExit(f"expected one compile command for {suffix}, found {len(matches)}")
    command = matches[0].get("command") or " ".join(matches[0]["arguments"])
    if "--offload-arch=gfx1151" not in command:
        raise SystemExit(f"gfx1151 architecture flag missing from {suffix}")
    macro = "-DGGML_HIP_ROCMFPX_QKV_Q8_REUSE"
    if mode == "on" and macro not in command:
        raise SystemExit(f"feature macro missing from feature-on {suffix}")
    if mode == "off" and macro in command:
        raise SystemExit(f"feature macro unexpectedly present in feature-off {suffix}")
    print(f"PASS: {mode} {suffix} architecture and macro contract")

libraries = sorted(build_dir.glob("**/libggml-hip.so*"))
if not libraries:
    raise SystemExit("linked libggml-hip.so not found")
print(f"PASS: {mode} linked {libraries[0]}")
PY

    cp "${build_dir}/CMakeCache.txt" "${evidence}/CMakeCache-${mode}.txt"
    cp "${build_dir}/compile_commands.json" "${evidence}/compile-commands-${mode}.json"
    cp "${build_dir}/CMakeFiles/CMakeConfigureLog.yaml" \
        "${evidence}/CMakeConfigureLog-${mode}.yaml"
    find "${build_dir}" -name 'libggml-hip.so*' -type f -print0 \
        | sort -z | xargs -0 sha256sum > "${evidence}/linked-libraries-${mode}.sha256"
done

(
    cd "${evidence}"
    find . -type f ! -name retained-evidence.sha256 -print0 \
        | sort -z | xargs -0 sha256sum > retained-evidence.sha256
)

echo 'PASS: exact pinned GPU-less gfx1151 HIP feature-OFF and feature-ON compile/link'
