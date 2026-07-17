#!/usr/bin/env bash
set -euo pipefail
BASE_URL="${BASE_URL:-http://127.0.0.1:9090}"
MODEL="${MODEL:-configured-model-name}"

curl --fail-with-body -sS "${BASE_URL}/models?reload=1"

curl --fail-with-body -sS -X POST \
  -H "Content-Type: application/json" \
  "${BASE_URL}/models/load" \
  -d "{\"model\":\"${MODEL}\"}"

curl --fail-with-body -sS -X POST \
  -H "Content-Type: application/json" \
  "${BASE_URL}/models/unload" \
  -d "{\"model\":\"${MODEL}\"}"
