#!/usr/bin/env bash
set -Eeuo pipefail

script_dir=$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd)
repo_root=$(cd -- "${script_dir}/../../../.." && pwd)
repo_root_windows=$(wslpath -w "${repo_root}")
build_dir="${repo_root}/build/qkv-host-evidence-d429a6e9"
output_dir="${script_dir}/host-wsl-ubuntu-26.04"
expected_head="d429a6e933dac620d91f58f58f8c27e3335ceefa"
actual_head=$(git.exe -C "${repo_root_windows}" rev-parse HEAD | tr -d '\r')

if [[ "${actual_head}" != "${expected_head}" ]]; then
    echo "refusing historical capture at ${actual_head}; expected ${expected_head}" >&2
    exit 1
fi

if [[ -e "${build_dir}" ]]; then
    echo "refusing to reuse evidence build directory: ${build_dir}" >&2
    exit 1
fi
if [[ -e "${output_dir}" ]]; then
    echo "refusing to overwrite evidence output directory: ${output_dir}" >&2
    exit 1
fi
mkdir -p "${output_dir}"

{
    printf 'captured_utc='
    date --utc --iso-8601=seconds
    uname -a
    cat /etc/os-release
    cmake --version
    ninja --version
    gcc --version
    g++ --version
} 2>&1 | tee "${output_dir}/toolchain.txt"

{
    git.exe -C "${repo_root_windows}" rev-parse HEAD
    git.exe -C "${repo_root_windows}" rev-parse HEAD~1
    git.exe -C "${repo_root_windows}" rev-parse HEAD~2
    git.exe -C "${repo_root_windows}" merge-base HEAD ad4930fd632f2f57bbe852dc2268ba3b5b7f5666
    git.exe -C "${repo_root_windows}" branch --show-current
    git.exe -C "${repo_root_windows}" status --porcelain=v2 --branch
    if git.exe -C "${repo_root_windows}" diff --quiet HEAD -- .github ggml tests; then
        echo 'implementation_paths_match_head=PASS'
    else
        echo 'implementation_paths_match_head=FAIL'
        exit 1
    fi
} 2>&1 | tee "${output_dir}/source-state.txt"

git.exe -C "${repo_root_windows}" ls-tree -r HEAD -- \
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
    tests/test-halofpx-rocmfpx-qkv-q8-reuse-source-contract.cmake \
    > "${output_dir}/implementation-blobs.txt"

if git.exe -C "${repo_root_windows}" diff --check > "${output_dir}/git-diff-check-output.txt" 2>&1; then
    echo 'PASS: git diff --check' >> "${output_dir}/git-diff-check-output.txt"
else
    exit 1
fi

cmake -S "${repo_root}" -B "${build_dir}" -G Ninja \
    -DCMAKE_BUILD_TYPE=Debug \
    -DCMAKE_EXPORT_COMPILE_COMMANDS=ON \
    -DBUILD_SHARED_LIBS=OFF \
    -DGGML_NATIVE=OFF \
    -DGGML_HIP=OFF \
    -DGGML_RPC=OFF \
    -DLLAMA_BUILD_TESTS=ON \
    -DLLAMA_BUILD_TOOLS=OFF \
    -DLLAMA_BUILD_EXAMPLES=OFF \
    -DLLAMA_BUILD_SERVER=OFF \
    -DLLAMA_BUILD_WEBUI=OFF \
    -DLLAMA_OPENSSL=OFF \
    2>&1 | tee "${output_dir}/configure-output.txt"

cmake --build "${build_dir}" --parallel 4 --target \
    test-halofpx-rocmfpx-qkv-q8-reuse-off \
    test-halofpx-rocmfpx-qkv-q8-reuse-on \
    test-backend-ops \
    2>&1 | tee "${output_dir}/build-output.txt"

ctest --test-dir "${build_dir}" --output-on-failure \
    -R '^test-halofpx-rocmfpx-qkv-q8-reuse-(off|on|source-contract)$' \
    2>&1 | tee "${output_dir}/qkv-contracts-output.txt"

"${build_dir}/bin/test-backend-ops" \
    -b CPU -o HALOFPX_ROCMFPX_QKV_Q8_REUSE \
    2>&1 | tee "${output_dir}/backend-qkv-cpu-output.txt"

"${build_dir}/bin/test-backend-ops" \
    -b CPU -o ADD -p 'type=f32' \
    2>&1 | tee "${output_dir}/backend-add-f32-output.txt"

cp "${build_dir}/CMakeCache.txt" "${output_dir}/CMakeCache.txt"
cp "${build_dir}/compile_commands.json" "${output_dir}/compile_commands.json"
cp "${build_dir}/CMakeFiles/CMakeConfigureLog.yaml" "${output_dir}/CMakeConfigureLog.yaml"
cp "${build_dir}/Testing/Temporary/LastTest.log" "${output_dir}/CTestLastTest.txt"
cp "${build_dir}/common/build-info.cpp" "${output_dir}/build-info.cpp"

sha256sum \
    "${build_dir}/bin/test-halofpx-rocmfpx-qkv-q8-reuse-off" \
    "${build_dir}/bin/test-halofpx-rocmfpx-qkv-q8-reuse-on" \
    "${build_dir}/bin/test-backend-ops" \
    > "${output_dir}/build-artifacts.sha256"

(
    cd "${output_dir}"
    find . -type f ! -name retained-evidence.sha256 -print0 |
        sort -z |
        xargs -0 sha256sum > retained-evidence.sha256
)

echo "evidence capture complete: ${output_dir}"
