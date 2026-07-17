#!/usr/bin/env bash
set -euo pipefail
if [[ $# -ne 5 ]]; then
  echo "usage: $0 LANE build|test SOURCE_DIR BUILD_DIR EVIDENCE_DIR" >&2
  exit 2
fi
lane=$1
phase=$2
source_dir=$3
build_dir=$4
evidence_dir=$5
mkdir -p "$evidence_dir"

cmake_args=(-DLLAMA_BUILD_TESTS=ON -DGGML_NATIVE=OFF)
case "$lane" in
  linux-rocm-dedicated)
    cmake_args+=(-DGGML_HIP=ON)
    ;;
  linux-vulkan-dedicated)
    cmake_args+=(-DGGML_VULKAN=ON)
    ;;
  linux-asan-ubsan)
    cmake_args+=(-DLLAMA_SANITIZE_ADDRESS=ON -DLLAMA_SANITIZE_UNDEFINED=ON)
    ;;
  *)
    echo "unknown lane $lane; add an explicit adapter" >&2
    exit 2
    ;;
esac

# Allow a reviewed lane definition to append fork-specific options.
if [[ -n ${CONFORMANCE_CMAKE_ARGS:-} ]]; then
  # shellcheck disable=SC2206
  extra=($CONFORMANCE_CMAKE_ARGS)
  cmake_args+=("${extra[@]}")
fi

case "$phase" in
  build)
    cmake -S "$source_dir" -B "$build_dir" "${cmake_args[@]}"
    cmake --build "$build_dir" --parallel
    cp "$build_dir/CMakeCache.txt" "$evidence_dir/CMakeCache.txt"
    ctest --test-dir "$build_dir" --show-only=json-v1 > "$evidence_dir/ctest-inventory.json"
    ;;
  test)
    ctest --test-dir "$build_dir" --output-on-failure | tee "$evidence_dir/ctest.log"
    ;;
  *)
    echo "phase must be build or test" >&2
    exit 2
    ;;
esac
