#!/usr/bin/env bash
set -uo pipefail
root=/home/connorb/halofpx-lab/open-pin-01
log=$root/nimo-2-reference-corrections.log
overall=0
run_cmd() {
 name="$1"; shift
 echo "===== $name ====="
 echo "argv=$*"
 "$@"
 rc=$?
 echo "rc=$rc"
 if [ "$rc" -ne 0 ]; then overall=1; fi
}
{
for lane in control candidate; do
 src=$root/$lane
 build=$src/build-open-pin-01
 echo "######## $lane $(git -C "$src" rev-parse HEAD) ########"
 run_cmd "$lane-rocmfp4-quant-regression-corrected" env ROOT="$src" BUILD_DIR="$build" "$src/scripts/check-rocmfp4-quant-regression.sh"
 run_cmd "$lane-list-ops-corrected" env LD_LIBRARY_PATH="$build/bin" "$build/bin/test-backend-ops" --list-ops
 done
echo "overall_rc=$overall"
} > "$log" 2>&1
printf '%s\n' "$overall" > "$root/nimo-2-reference-corrections.rc"
exit "$overall"