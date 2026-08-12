#!/usr/bin/env bash
set -euo pipefail
ROOT="${1:?usage: inventory-artifact.sh ROOT OUTPUT_DIR}"
OUT="${2:?usage: inventory-artifact.sh ROOT OUTPUT_DIR}"
ROOT="$(readlink -f "$ROOT")"
mkdir -p "$OUT"

find "$ROOT" -type f -printf '%P\0' | LC_ALL=C sort -z | tr '\0' '\n' > "$OUT/files.txt"
(
  cd "$ROOT"
  find . -type f -print0 | LC_ALL=C sort -z | xargs -0 sha256sum
) > "$OUT/files.sha256"
find "$ROOT" -type f \( -iname 'LICENSE*' -o -iname 'COPYING*' -o -iname 'NOTICE*' -o -iname '*copyright*' \) -printf '%P\n' | LC_ALL=C sort > "$OUT/license-files.txt"
find "$ROOT" -type f -perm /111 -printf '%P\n' | LC_ALL=C sort > "$OUT/executables.txt"
find "$ROOT" -type f \( -name '*.so' -o -name '*.so.*' -o -perm /111 \) -print0 | while IFS= read -r -d '' f; do
  file "$f"
done > "$OUT/file-types.txt" || true

if command -v readelf >/dev/null; then
  while IFS= read -r rel; do
    f="$ROOT/$rel"
    file "$f" | grep -q ELF || continue
    echo "### $rel"
    readelf -d "$f" 2>/dev/null | grep -E 'NEEDED|RPATH|RUNPATH|SONAME' || true
  done < "$OUT/files.txt" > "$OUT/elf-dynamic.txt"
fi

for c in amdclang clang hipcc; do
  if [[ -x "$ROOT/bin/$c" ]]; then
    "$ROOT/bin/$c" --version > "$OUT/$c-version.txt" 2>&1 || true
    "$ROOT/bin/$c" --print-resource-dir > "$OUT/$c-resource-dir.txt" 2>&1 || true
  fi
done
find "$ROOT" -type d -path '*/amdgcn/bitcode' -print > "$OUT/device-library-dirs.txt"
find "$ROOT" -type d -path '*/lib/clang/*' -print > "$OUT/clang-resource-candidates.txt"
find "$ROOT" -path '*/share/therock/therock_manifest.json' -type f -exec cp '{}' "$OUT/therock_manifest.json" ';' -quit || true
printf '%s\n' "$(date -u +%FT%TZ)" > "$OUT/inventory-at-utc.txt"
