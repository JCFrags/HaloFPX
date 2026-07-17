#!/usr/bin/env bash
set -euo pipefail
BASE_URL="${BASE_URL:-http://127.0.0.1:9090}"
API_KEY="${API_KEY:-change-me}"

curl --fail-with-body --silent --show-error \
  -H "Authorization: Bearer ${API_KEY}" \
  -H "anthropic-version: 2023-06-01" \
  -H "Content-Type: application/json" \
  "${BASE_URL}/v1/messages" \
  -d '{
    "model": "configured-model-name",
    "max_tokens": 128,
    "system": "Answer with compact, auditable reasoning.",
    "messages": [{"role": "user", "content": "Summarize the cache design."}],
    "metadata": {"user_id": "tenant42-user7"}
  }'
