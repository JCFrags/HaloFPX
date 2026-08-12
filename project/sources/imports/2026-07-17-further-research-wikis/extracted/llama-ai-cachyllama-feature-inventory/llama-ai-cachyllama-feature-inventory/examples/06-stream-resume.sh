#!/usr/bin/env bash
set -euo pipefail
BASE_URL="${BASE_URL:-http://127.0.0.1:9090}"
CONVERSATION_ID="${CONVERSATION_ID:?set CONVERSATION_ID}"

curl --fail-with-body -sS \
  "${BASE_URL}/v1/stream/${CONVERSATION_ID}"

curl --fail-with-body -sS -X DELETE \
  "${BASE_URL}/v1/stream/${CONVERSATION_ID}"
