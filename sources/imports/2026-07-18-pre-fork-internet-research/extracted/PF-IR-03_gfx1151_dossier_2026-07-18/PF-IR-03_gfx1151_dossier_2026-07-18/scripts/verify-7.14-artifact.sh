#!/usr/bin/env bash
set -euo pipefail

URL="https://repo.amd.com/rocm/tarball-multi-arch/therock-dist-linux-gfx1151-7.14.0.tar.gz"
OUT="${1:-$PWD/pf-ir-03-acquire}"
EXPECTED_SHA256="${EXPECTED_SHA256:-${2:-}}"
ALLOW_UNAUTHENTICATED="${ALLOW_UNAUTHENTICATED:-0}"
mkdir -p "$OUT"
ARCHIVE="$OUT/therock-dist-linux-gfx1151-7.14.0.tar.gz"
HEADERS="$OUT/http-headers.txt"

command -v curl >/dev/null || { echo 'curl is required' >&2; exit 2; }
command -v sha256sum >/dev/null || exit 2
command -v sha512sum >/dev/null || exit 2
command -v tar >/dev/null || exit 2

curl --fail --location --proto '=https' --tlsv1.2 --dump-header "$HEADERS" --output "$ARCHIVE" "$URL"
sha256sum "$ARCHIVE" | tee "$OUT/archive.sha256"
sha512sum "$ARCHIVE" | tee "$OUT/archive.sha512"
ACTUAL_SHA256="$(cut -d' ' -f1 "$OUT/archive.sha256")"

if [[ -z "$EXPECTED_SHA256" ]]; then
  echo '[PROVENANCE_GAP] No authenticated expected SHA-256 supplied.' | tee "$OUT/INTEGRITY-STATUS.txt"
  if [[ "$ALLOW_UNAUTHENTICATED" != 1 ]]; then
    echo 'Refusing extraction/promotion. Set ALLOW_UNAUTHENTICATED=1 only for quarantined inventory.' >&2
    exit 3
  fi
else
  [[ "$ACTUAL_SHA256" == "$EXPECTED_SHA256" ]] || { echo 'SHA-256 mismatch' >&2; exit 4; }
  echo '[VERIFIED_EXPECTED_DIGEST] SHA-256 matched supplied value.' | tee "$OUT/INTEGRITY-STATUS.txt"
fi

tar -tzf "$ARCHIVE" | LC_ALL=C sort > "$OUT/tar-list.txt"
if awk 'BEGIN{bad=0} /^\//{bad=1} /(^|\/)\.\.($|\/)/{bad=1} END{exit bad?0:1}' "$OUT/tar-list.txt"; then
  echo 'Unsafe archive path found' >&2
  exit 5
fi

EXTRACT="$OUT/extracted"
mkdir -p "$EXTRACT"
tar -xzf "$ARCHIVE" -C "$EXTRACT" --no-same-owner
find "$EXTRACT" -type f -print0 | LC_ALL=C sort -z | xargs -0 sha256sum > "$OUT/extracted-files.sha256"
find "$EXTRACT" -type f \( -iname 'LICENSE*' -o -iname 'COPYING*' -o -iname 'NOTICE*' -o -iname '*copyright*' \) -print | LC_ALL=C sort > "$OUT/license-candidates.txt"
MANIFEST="$(find "$EXTRACT" -path '*/share/therock/therock_manifest.json' -type f -print -quit || true)"
if [[ -n "$MANIFEST" ]]; then
  cp "$MANIFEST" "$OUT/therock_manifest.json"
  python3 -m json.tool "$MANIFEST" > "$OUT/therock_manifest.pretty.json"
else
  echo '[PROVENANCE_GAP] therock_manifest.json not found' > "$OUT/manifest-status.txt"
fi
printf '%s\n' "$URL" > "$OUT/source-url.txt"
printf '%s\n' "$(date -u +%FT%TZ)" > "$OUT/acquired-at-utc.txt"
echo "Inventory complete: $OUT"
