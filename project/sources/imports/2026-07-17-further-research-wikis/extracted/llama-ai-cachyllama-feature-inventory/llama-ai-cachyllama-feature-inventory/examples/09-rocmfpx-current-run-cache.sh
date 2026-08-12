#!/usr/bin/env bash
set -euo pipefail
MODEL="${MODEL:?set MODEL to a GGUF path}"
CACHE_ROOT="${CACHE_ROOT:-$PWD/.cache/rocmfpx-run}"

exec ./build/bin/llama-server \
  --model "${MODEL}" \
  --cache-ram 8192 \
  --cache-disk "${CACHE_ROOT}" \
  --cache-disk-limit 8192 \
  --metrics
