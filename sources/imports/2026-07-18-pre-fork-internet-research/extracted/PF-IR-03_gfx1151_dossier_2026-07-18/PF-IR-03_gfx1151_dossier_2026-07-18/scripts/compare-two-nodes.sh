#!/usr/bin/env bash
set -euo pipefail
A="${1:?usage: compare-two-nodes.sh NODE_A_CAPTURE NODE_B_CAPTURE [OUT]}"
B="${2:?usage: compare-two-nodes.sh NODE_A_CAPTURE NODE_B_CAPTURE [OUT]}"
OUT="${3:-$PWD/two-node-comparison}"
mkdir -p "$OUT"
A="$(readlink -f "$A")"; B="$(readlink -f "$B")"

common=(os-release uname kernel-version cmdline amdgpu-modinfo amdkfd-modinfo hipconfig amdclang-version amdclang-resource rocm-symlinks kernel-config-relevant dpkg-relevant rpm-relevant amdgpu-firmware.sha256)
status=0
for name in "${common[@]}"; do
  af="$A/$name.txt"; bf="$B/$name.txt"
  [[ "$name" == *.sha256 ]] && { af="$A/$name"; bf="$B/$name"; }
  if [[ -e "$af" || -e "$bf" ]]; then
    diff -u "$af" "$bf" > "$OUT/$name.diff" 2>&1 || status=1
  fi
done

find "$A" -maxdepth 1 -type f -printf '%f\n' | sort > "$OUT/node-a-files.txt"
find "$B" -maxdepth 1 -type f -printf '%f\n' | sort > "$OUT/node-b-files.txt"
diff -u "$OUT/node-a-files.txt" "$OUT/node-b-files.txt" > "$OUT/file-set.diff" || status=1
printf '%s\n' "$status" > "$OUT/differences-present.txt"
if [[ "$status" == 0 ]]; then
  echo '[CAPTURE_EQUIVALENT_FOR_CHECKED_FIELDS]' > "$OUT/STATUS.txt"
else
  echo '[NODE_DRIFT_DETECTED]' > "$OUT/STATUS.txt"
fi
exit "$status"
