#!/usr/bin/env bash
set -uo pipefail
root=/home/connorb/halofpx-lab/open-pin-01
log=$root/nimo-2-fattn-ext-rocm0.log
overall=0
{
echo "start=$(date --iso-8601=ns)"
rocm-smi --showtemp --showuse --showmemuse
for lane in control candidate; do
 src=$root/$lane
 build=$src/build-open-pin-01
 echo "######## $lane $(git -C "$src" rev-parse HEAD) ########"
 echo "argv=$build/bin/test-backend-ops test -o FLASH_ATTN_EXT -b ROCm0 --output console"
 env LD_LIBRARY_PATH="$build/bin" "$build/bin/test-backend-ops" test -o FLASH_ATTN_EXT -b ROCm0 --output console
 rc=$?
 echo "rc=$rc"
 if [ "$rc" -ne 0 ]; then overall=1; fi
 rocm-smi --showtemp --showuse --showmemuse
 done
journalctl -k --since "2026-07-17 18:23:00" -p warning..alert --no-pager
echo "end=$(date --iso-8601=ns)"
echo "overall_rc=$overall"
} > "$log" 2>&1
printf '%s\n' "$overall" > "$root/nimo-2-fattn-ext-rocm0.rc"
exit "$overall"