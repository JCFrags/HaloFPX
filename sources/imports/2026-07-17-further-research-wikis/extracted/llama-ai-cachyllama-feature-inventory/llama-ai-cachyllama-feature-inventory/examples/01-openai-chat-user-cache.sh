#!/usr/bin/env bash
set -euo pipefail
BASE_URL="${BASE_URL:-http://127.0.0.1:9090}"
API_KEY="${API_KEY:-change-me}"

curl --fail-with-body --silent --show-error \
  -H "Authorization: Bearer ${API_KEY}" \
  -H "Content-Type: application/json" \
  "${BASE_URL}/v1/chat/completions" \
  -d '{
    "model": "configured-model-name",
    "messages": [
      {"role": "system", "content": "Answer with compact, auditable reasoning."},
      {"role": "user", "content": "Summarize the cache design."}
    ],
    "stream": false,
    "cache_prompt": true,
    "llama_user_id": "tenant42-user7"
  }'
