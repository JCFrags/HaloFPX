#!/usr/bin/env bash
set -Eeuo pipefail

repo=/workspace/src
work=/workspace/work
evidence=/workspace/evidence
expected_head=609c166421ecf3eecaa67340e4f40fcb750a0f48
expected_parent=3d9a0c3cc52168f696d600099742c7caf964161f

mkdir -p "${work}" "${evidence}"

finish_failure() {
    rc=$?
    set +e
    printf '%s\n' "${rc}" > "${evidence}/runner-exit-code.txt"
    date --utc --iso-8601=seconds > "${evidence}/runner-finished-utc.txt"
    touch "${evidence}/runner-FAIL"
    exit "${rc}"
}
trap finish_failure EXIT

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

test ! -e /dev/kfd
test ! -e /dev/dri
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
actual_parent=$(git -C "${repo}" rev-parse HEAD^)
test "${actual_head}" = "${expected_head}"
test "${actual_parent}" = "${expected_parent}"
test -z "$(git -C "${repo}" status --porcelain)"
{
    printf 'source_head=%s\n' "${actual_head}"
    printf 'source_parent=%s\n' "${actual_parent}"
    git -C "${repo}" status --porcelain=v2 --branch
    git -C "${repo}" ls-tree -r HEAD -- \
        .github/workflows/halofpx-ci.yml \
        HANDOFF.md \
        docs/halofpx/README.md \
        docs/halofpx/decisions/README.md \
        docs/halofpx/decisions/0053-rocmfpx-qkv-q8-activation-reuse.md \
        docs/halofpx/decisions/0055-rocmfpx-strict-n1-mmvq-qkv-q8-reuse.md \
        docs/halofpx/decisions/0060-rocmfpx-routed-moe-q8-reuse.md \
        docs/halofpx/rocmfpx-qkv-q8-reuse.md \
        docs/halofpx/rocmfpx-mmvq-qkv-q8-reuse.md \
        docs/halofpx/rocmfpx-routed-moe-q8-reuse.md \
        docs/halofpx/rocmfpx-prefill-candidate-screen-9bfccf25.md \
        ggml/CMakeLists.txt \
        ggml/src/ggml-hip/CMakeLists.txt \
        ggml/src/ggml-cuda/common.cuh \
        ggml/src/ggml-cuda/ggml-cuda.cu \
        ggml/src/ggml-cuda/mmq.cu \
        ggml/src/ggml-cuda/mmq.cuh \
        ggml/src/ggml-cuda/mmvq.cu \
        ggml/src/ggml-cuda/mmvq.cuh \
        ggml/src/ggml-cuda/rocmfpx-qkv-q8-reuse.h \
        ggml/src/ggml-cuda/rocmfpx-mmvq-qkv-q8-reuse.h \
        ggml/src/ggml-cuda/rocmfpx-moe-q8-reuse.h \
        scripts/build-halofpx-primary-matched.sh \
        scripts/build-strix-rocmfp4-mtp.sh \
        tests/CMakeLists.txt \
        tests/test-backend-ops.cpp \
        tests/test-halofpx-rocmfpx-qkv-q8-reuse.cpp \
        tests/test-halofpx-rocmfpx-qkv-q8-reuse-source-contract.cmake \
        tests/test-halofpx-rocmfpx-mmvq-qkv-q8-reuse.cpp \
        tests/test-halofpx-rocmfpx-mmvq-qkv-q8-reuse-source-contract.cmake \
        tests/test-halofpx-rocmfpx-moe-q8-reuse.cpp \
        tests/test-halofpx-rocmfpx-moe-q8-reuse-source-contract.cmake \
        tests/test-halofpx-rocmfpx-qkv-moe-composition.cpp
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
        -DGGML_HIP_ROCMFPX_MMVQ_QKV_Q8_REUSE="${option}" \
        -DGGML_HIP_ROCMFPX_MOE_Q8_REUSE="${option}" \
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
macros = (
    "-DGGML_HIP_ROCMFPX_QKV_Q8_REUSE",
    "-DGGML_HIP_ROCMFPX_MMVQ_QKV_Q8_REUSE",
    "-DGGML_HIP_ROCMFPX_MOE_Q8_REUSE",
)
for suffix in (
    "/ggml/src/ggml-cuda/ggml-cuda.cu",
    "/ggml/src/ggml-cuda/mmq.cu",
    "/ggml/src/ggml-cuda/mmvq.cu",
):
    matches = [entry for entry in entries if entry["file"].replace("\\", "/").endswith(suffix)]
    if len(matches) != 1:
        raise SystemExit(f"expected one compile command for {suffix}, found {len(matches)}")
    command = matches[0].get("command") or " ".join(matches[0]["arguments"])
    if "--offload-arch=gfx1151" not in command:
        raise SystemExit(f"gfx1151 architecture flag missing from {suffix}")
    for macro in macros:
        if mode == "on" and macro not in command:
            raise SystemExit(f"{macro} missing from feature-on {suffix}")
        if mode == "off" and macro in command:
            raise SystemExit(f"{macro} unexpectedly present in feature-off {suffix}")
    if "-DGGML_HIP_ROCMFPX_FFN_Q8_REUSE" in command:
        raise SystemExit(f"dense FFN macro unexpectedly present in {suffix}")
    print(f"PASS: {mode} {suffix} gfx1151 and three-feature macro contract")

cache = (build_dir / "CMakeCache.txt").read_text()
expected = "ON" if mode == "on" else "OFF"
for option in (
    "GGML_HIP_ROCMFPX_QKV_Q8_REUSE",
    "GGML_HIP_ROCMFPX_MMVQ_QKV_Q8_REUSE",
    "GGML_HIP_ROCMFPX_MOE_Q8_REUSE",
):
    line = f"{option}:BOOL={expected}"
    if line not in cache:
        raise SystemExit(f"missing cache receipt: {line}")
    print(f"PASS: {mode} cache {line}")
if "GGML_HIP_ROCMFPX_FFN_Q8_REUSE:BOOL=OFF" not in cache:
    raise SystemExit("dense FFN option is not pinned OFF")
print("PASS: dense FFN option pinned OFF")

libraries = sorted(path for path in build_dir.glob("**/libggml-hip.so*") if path.is_file())
if not libraries:
    raise SystemExit("linked regular libggml-hip.so not found")
print(f"PASS: {mode} linked {libraries[0]}")
PY

    cp "${build_dir}/CMakeCache.txt" "${evidence}/CMakeCache-${mode}.txt"
    cp "${build_dir}/compile_commands.json" "${evidence}/compile-commands-${mode}.json"
    cp "${build_dir}/CMakeFiles/CMakeConfigureLog.yaml" \
        "${evidence}/CMakeConfigureLog-${mode}.yaml"
    find "${build_dir}" -name 'libggml-hip.so*' -type f -print0 \
        | sort -z | xargs -0 sha256sum > "${evidence}/linked-libraries-${mode}.sha256"
done

test -z "$(git -C "${repo}" status --porcelain)"
printf '0\n' > "${evidence}/runner-exit-code.txt"
date --utc --iso-8601=seconds > "${evidence}/runner-finished-utc.txt"
touch "${evidence}/runner-PASS"
trap - EXIT
echo 'PASS: exact pinned GPU-less gfx1151 HIP composed OFF/OFF/OFF and ON/ON/ON compile/link'
