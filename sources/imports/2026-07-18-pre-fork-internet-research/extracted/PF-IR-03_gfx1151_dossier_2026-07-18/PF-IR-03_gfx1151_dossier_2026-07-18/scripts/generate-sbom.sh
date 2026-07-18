#!/usr/bin/env bash
set -euo pipefail
ROOT="${1:?usage: generate-sbom.sh ROOT OUTPUT_DIR}"
OUT="${2:?usage: generate-sbom.sh ROOT OUTPUT_DIR}"
mkdir -p "$OUT"
if command -v syft >/dev/null; then
  syft "dir:$ROOT" -o spdx-json="$OUT/sbom.spdx.json" -o cyclonedx-json="$OUT/sbom.cdx.json"
  syft version > "$OUT/syft-version.txt"
else
  echo 'syft is not installed; no complete SBOM generated.' >&2
  echo '[SBOM_NOT_GENERATED]' > "$OUT/SBOM-STATUS.txt"
  exit 3
fi
