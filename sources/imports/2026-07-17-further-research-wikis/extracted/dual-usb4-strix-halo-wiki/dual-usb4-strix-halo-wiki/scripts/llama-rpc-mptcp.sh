#!/usr/bin/env bash
set -Eeuo pipefail
SCRIPT_DIR=$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd)
source "$SCRIPT_DIR/common.sh"

[[ $# -gt 0 ]] || die 'usage: llama-rpc-mptcp.sh COMMAND [ARGS...]'
command -v mptcpize >/dev/null 2>&1 || die 'mptcpize not found; install the MPTCP userspace tools or use the native patch'

ip mptcp endpoint show || true
ip mptcp limits show || true
export GGML_RPC_DEBUG=${GGML_RPC_DEBUG:-1}
log 'starting command through mptcpize; verify live sockets with ss -Mani'
exec mptcpize run "$@"
