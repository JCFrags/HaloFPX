#!/usr/bin/env bash

set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
ROOT="$(cd "$SCRIPT_DIR/.." && pwd)"

# shellcheck source=../scripts/halofpx-rocmfpx-mmvq-process-guard.sh
source "$ROOT/scripts/halofpx-rocmfpx-mmvq-process-guard.sh"

positive_cases=(
    '/opt/llm/llama-server --port 8081'
    '/tmp/build/bin/llama-cli -m model.gguf'
    '/tmp/build/bin/llama-completion -m model.gguf'
    '/tmp/build/bin/llama-bench -m model.gguf'
    '/tmp/build/bin/rpc-server --port 50052'
    '/opt/llm-usb4-cluster/llama/ggml-rpc-server --host 0.0.0.0 --port 50052'
    'ggml-rpc-server --port 50052'
)

negative_cases=(
    '/opt/llm/llama-server-monitor --watch'
    '/tmp/build/bin/ggml-rpc-server-helper --port 50052'
    '/tmp/build/bin/my-rpc-server --port 50052'
    'python3 worker.py --name ggml-rpc-server'
    '/tmp/build/bin/test-backend-ops test -b ROCm0'
)

for command_line in "${positive_cases[@]}"; do
    if ! halofpx_rocmfpx_mmvq_process_matches "$command_line"; then
        printf 'expected active inference process match: %s\n' "$command_line" >&2
        exit 1
    fi
done

for command_line in "${negative_cases[@]}"; do
    if halofpx_rocmfpx_mmvq_process_matches "$command_line"; then
        printf 'unexpected active inference process match: %s\n' "$command_line" >&2
        exit 1
    fi
done

printf 'ROCmFPX MMVQ process guard: %d positive and %d negative cases passed\n' \
    "${#positive_cases[@]}" "${#negative_cases[@]}"
