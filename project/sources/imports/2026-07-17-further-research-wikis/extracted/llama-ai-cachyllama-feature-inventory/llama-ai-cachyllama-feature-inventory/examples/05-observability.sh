#!/usr/bin/env bash
set -euo pipefail
BASE_URL="${BASE_URL:-http://127.0.0.1:9090}"

curl --fail-with-body -sS "${BASE_URL}/health"
curl --fail-with-body -sS "${BASE_URL}/props"
curl --fail-with-body -sS "${BASE_URL}/slots"
curl --fail-with-body -sS "${BASE_URL}/metrics"
curl --fail-with-body -sS "${BASE_URL}/expert-stats"
