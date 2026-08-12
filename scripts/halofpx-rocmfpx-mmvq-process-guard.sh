#!/usr/bin/env bash

# Exact executable basenames which can own llama.cpp/ggml GPU work on a target.
# The surrounding path/start and whitespace/end anchors avoid matching helper
# names such as `ggml-rpc-server-monitor`.
HALOFPX_ROCMFPX_MMVQ_ACTIVE_PROCESS_RE='(^|/)(llama-server|llama-cli|llama-completion|llama-bench|rpc-server|ggml-rpc-server)([[:space:]]|$)'

halofpx_rocmfpx_mmvq_process_matches() {
    local command_line="${1:?command line is required}"
    [[ "$command_line" =~ $HALOFPX_ROCMFPX_MMVQ_ACTIVE_PROCESS_RE ]]
}

halofpx_rocmfpx_mmvq_active_processes() {
    pgrep -af "$HALOFPX_ROCMFPX_MMVQ_ACTIVE_PROCESS_RE" || true
}
