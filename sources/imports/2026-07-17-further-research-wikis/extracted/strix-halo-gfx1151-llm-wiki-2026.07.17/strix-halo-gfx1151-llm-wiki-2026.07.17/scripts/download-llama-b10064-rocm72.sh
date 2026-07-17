#!/usr/bin/env bash
set -euo pipefail

VERSION="b10064"
ASSET="llama-b10064-bin-ubuntu-rocm-7.2-x64.tar.gz"
URL="https://github.com/ggml-org/llama.cpp/releases/download/${VERSION}/${ASSET}"
SHA256="42a00452f42b04598d32db66c5249b3e8855cd99bf9448e22fd2a738aaa89c82"
OUT_DIR="${OUT_DIR:-$PWD/downloads/llama-b10064-rocm72}"
CACHE_DIR="${CACHE_DIR:-$HOME/.cache/strix-halo-wiki}"
mkdir -p "$CACHE_DIR" "$(dirname "$OUT_DIR")"
ARCHIVE="$CACHE_DIR/$ASSET"

if [[ ! -f "$ARCHIVE" ]]; then
    curl --fail --location --retry 3 --continue-at - "$URL" -o "$ARCHIVE"
fi
printf '%s  %s\n' "$SHA256" "$ARCHIVE" | sha256sum -c -
STAGE="$(mktemp -d)"
trap 'rm -rf "$STAGE"' EXIT
tar -xf "$ARCHIVE" -C "$STAGE"
rm -rf "$OUT_DIR"
mkdir -p "$OUT_DIR"
mapfile -d '' TOP < <(find "$STAGE" -mindepth 1 -maxdepth 1 -print0)
if [[ ${#TOP[@]} -eq 1 && -d "${TOP[0]}" ]]; then cp -a "${TOP[0]}/." "$OUT_DIR/"; else cp -a "$STAGE/." "$OUT_DIR/"; fi
cat >"$OUT_DIR/DOWNLOAD-PROVENANCE.txt" <<EOF_PROV
release=$VERSION
url=$URL
sha256=$SHA256
verified_at=$(date --iso-8601=seconds 2>/dev/null || date)
EOF_PROV
[[ -x "$OUT_DIR/llama-cli" ]] || find "$OUT_DIR" -maxdepth 3 -type f -name llama-cli -print
echo "Verified and extracted to $OUT_DIR"
