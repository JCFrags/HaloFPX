#!/usr/bin/env bash
set -euo pipefail
BASE_URL="${BASE_URL:-http://127.0.0.1:9090}"
SLOT_ID="${SLOT_ID:-0}"
FILE="${FILE:-review-session.bin}"

curl --fail-with-body -sS -X POST \
  -H "Content-Type: application/json" \
  "${BASE_URL}/slots/${SLOT_ID}?action=save" \
  -d "{\"filename\":\"${FILE}\"}"

curl --fail-with-body -sS -X POST \
  -H "Content-Type: application/json" \
  "${BASE_URL}/slots/${SLOT_ID}?action=restore" \
  -d "{\"filename\":\"${FILE}\"}"

curl --fail-with-body -sS -X POST \
  "${BASE_URL}/slots/${SLOT_ID}?action=erase"
