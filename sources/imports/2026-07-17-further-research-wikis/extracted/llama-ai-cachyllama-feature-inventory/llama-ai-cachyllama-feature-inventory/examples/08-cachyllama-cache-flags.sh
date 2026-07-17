#!/usr/bin/env bash
set -euo pipefail
MODEL="${MODEL:?set MODEL to a GGUF path}"
CACHE_ROOT="${CACHE_ROOT:-$PWD/.cache/cachyllama}"
SLOT_ROOT="${SLOT_ROOT:-$PWD/.cache/slots}"

exec ./build/bin/llama-server \
  --model "${MODEL}" \
  --host 127.0.0.1 --port 9090 \
  --metrics --slots \
  --slot-save-path "${SLOT_ROOT}/" \
  --cache-ssd "${CACHE_ROOT}" \
  --cache-ssd-checkpoints 64 \
  --cache-ssd-hot-window 16384 \
  --cache-ssd-warm-window 32768 \
  --cache-ssd-max-cold 0 \
  --cache-ssd-system-prompts 8 \
  --cache-ssd-system-max-days 30 \
  --max-concurrent-per-user 2
