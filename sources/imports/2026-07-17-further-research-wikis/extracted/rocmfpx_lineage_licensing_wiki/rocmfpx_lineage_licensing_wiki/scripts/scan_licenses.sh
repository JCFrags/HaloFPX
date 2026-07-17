#!/usr/bin/env bash
set -euo pipefail

ROOT="${1:?usage: scan_licenses.sh CHECKOUT [OUTPUT_DIR]}"
OUT="${2:-license-audit}"
mkdir -p "$OUT"
ROOT="$(cd "$ROOT" && pwd)"
OUT="$(cd "$OUT" && pwd)"

find "$ROOT" -type f \( -iname 'LICENSE*' -o -iname 'COPYING*' -o -iname 'NOTICE*' \) \
  -not -path '*/.git/*' -print | sort > "$OUT/license-files.txt"

grep -RInE --exclude-dir=.git --exclude='*.lock' \
  'SPDX-License-Identifier|Licensed under|Public Domain|Unlicense|Copyright' \
  "$ROOT" > "$OUT/license-markers.txt" || true

if [[ -d "$ROOT/.git" ]]; then
  git -C "$ROOT" ls-tree -r HEAD | awk '$1 == "160000" {print}' > "$OUT/gitlinks.txt"
  git -C "$ROOT" status --porcelain=v1 > "$OUT/worktree-status.txt"
fi

if command -v reuse >/dev/null 2>&1; then
  (cd "$ROOT" && reuse lint) > "$OUT/reuse-lint.txt" 2>&1 || true
fi
if command -v scancode >/dev/null 2>&1; then
  scancode --license --copyright --package --json-pp "$OUT/scancode.json" "$ROOT" || true
fi
if command -v syft >/dev/null 2>&1; then
  syft "dir:$ROOT" -o spdx-json="$OUT/source.spdx.json" || true
fi
if command -v licensee >/dev/null 2>&1; then
  licensee detect "$ROOT" > "$OUT/licensee.txt" 2>&1 || true
fi

printf 'Audit output: %s\n' "$OUT"
printf 'Manual review remains required for unknown, generated, binary, archive, model, and dual-license files.\n'
