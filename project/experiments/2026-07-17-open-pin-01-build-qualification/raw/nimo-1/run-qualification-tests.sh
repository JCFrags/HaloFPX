#!/usr/bin/env bash
set -uo pipefail
root=/home/connorb/halofpx-lab/open-pin-01
log=$root/nimo-1-qualification-tests.log
overall=0
run_cmd() {
 name="$1"; shift
 echo "===== $name ====="
 echo "argv=$*"
 echo "start=$(date --iso-8601=ns)"
 "$@"
 rc=$?
 echo "rc=$rc"
 echo "end=$(date --iso-8601=ns)"
 if [ "$rc" -ne 0 ]; then overall=1; fi
}
{
echo "run_start=$(date --iso-8601=ns)"
rocm-smi --showtemp --showuse --showmemuse
for lane in control candidate; do
 src=$root/$lane
 build=$src/build-open-pin-01
 echo "######## $lane $(git -C "$src" rev-parse HEAD) ########"
 run_cmd "$lane-build-test-turboquant" cmake --build "$build" -j 16 --target test-turboquant
 run_cmd "$lane-rocmfp2-reference" env ROOT="$src" BUILD_DIR="$src/build-ref-rocmfp2" "$src/scripts/check-rocmfp2-reference.sh"
 run_cmd "$lane-rocmfpx-reference" env ROOT="$src" BUILD_DIR="$src/build-ref-rocmfpx" "$src/scripts/check-rocmfpx-reference.sh"
 run_cmd "$lane-rocmfp4-quant-regression" env ROOT="$src" BUILD_DIR="$build" "$src/scripts/check-rocmfp4-quant-regression.sh"
 run_cmd "$lane-test-turboquant" env LD_LIBRARY_PATH="$build/bin" "$build/bin/test-turboquant"
 run_cmd "$lane-fattn-ext-rocm0" env LD_LIBRARY_PATH="$build/bin" "$build/bin/test-backend-ops" test -o FLASH_ATTN_EXT -b ROCm0 --output console
 echo "===== $lane artifact hashes ====="
 find "$build/bin" -maxdepth 1 -type f -executable -print0 | sort -z | xargs -0 sha256sum
 echo "===== $lane cmake cache ====="
 sha256sum "$build/CMakeCache.txt"
 grep -E '^(CMAKE_BUILD_TYPE|CMAKE_HIP_ARCHITECTURES|GPU_TARGETS|GGML_HIP|GGML_VULKAN|GGML_CUDA|LLAMA_BUILD_SERVER|LLAMA_BUILD_TESTS|GGML_BUILD_TESTS):' "$build/CMakeCache.txt" | sort
 rocm-smi --showtemp --showuse --showmemuse
 done
free -b
swapon --show --bytes
df -B1 /
journalctl -k --since "2026-07-17 18:22:00" -p warning..alert --no-pager
echo "overall_rc=$overall"
echo "run_end=$(date --iso-8601=ns)"
} > "$log" 2>&1
printf '%s\n' "$overall" > "$root/nimo-1-qualification-tests.rc"
exit "$overall"