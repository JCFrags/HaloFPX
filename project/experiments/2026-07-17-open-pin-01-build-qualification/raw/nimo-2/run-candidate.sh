#!/usr/bin/env bash
set -uo pipefail
root=/home/connorb/halofpx-lab/open-pin-01
src=$root/candidate
log=$root/candidate-build.log
rcfile=$root/candidate-build.rc
{
  echo "start=$(date --iso-8601=ns)"
  echo "commit=$(git -C "$src" rev-parse HEAD)"
  echo "tree=$(git -C "$src" rev-parse HEAD^{tree})"
  echo "status_begin=$(git -C "$src" status --porcelain=v1 | wc -l)"
  uname -a
  cmake --version
  ninja --version
  hipcc --version
  clang --version
  free -b
  swapon --show --bytes
  df -B1 /
  echo "build_command=env JOBS=16 BUILD_DIR=$src/build-open-pin-01 scripts/build-strix-rocmfp4-mtp.sh"
  cd "$src"
  time -p env JOBS=16 BUILD_DIR="$src/build-open-pin-01" scripts/build-strix-rocmfp4-mtp.sh
  rc=$?
  echo "build_rc=$rc"
  echo "status_end=$(git -C "$src" status --porcelain=v1 | wc -l)"
  free -b
  swapon --show --bytes
  df -B1 /
  journalctl -k --since "2026-07-17 18:18:55" -p warning..alert --no-pager
  echo "end=$(date --iso-8601=ns)"
} > "$log" 2>&1
printf '%s\n' "$rc" > "$rcfile"
exit "$rc"