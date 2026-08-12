#!/usr/bin/env bash
set -Eeuo pipefail
SCRIPT_DIR=$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd)
source "$SCRIPT_DIR/common.sh"

require_root
require_cmd ip
ROLE=${ROLE:-}
IF0=${IF0:-thunderbolt0}
IF1=${IF1:-thunderbolt1}
ENDPOINT_ID=${ENDPOINT_ID:-2}
SUBFLOWS=${SUBFLOWS:-2}
ADD_ADDR_ACCEPTED=${ADD_ADDR_ACCEPTED:-2}
LAMINAR=${LAMINAR:-1}
role_addresses
validate_iface "$IF0"
validate_iface "$IF1"
MPTCP_HELP=$(ip mptcp help 2>&1 || true)
grep -q 'endpoint add' <<<"$MPTCP_HELP" || die 'iproute2 lacks MPTCP endpoint support'
if [[ $ROLE == client || $ROLE == A || $ROLE == a ]] && [[ $LAMINAR == 1 ]] && ! grep -qw laminar <<<"$MPTCP_HELP"; then
    die 'this iproute2 build lacks the laminar endpoint flag; upgrade iproute2 or rerun with LAMINAR=0'
fi

if [[ ${RESET_ENDPOINTS:-0} == 1 ]]; then
    warn 'deleting all existing MPTCP endpoints in this network namespace'
    run ip mptcp endpoint flush
else
    if ip mptcp endpoint show | grep -q .; then
        warn 'existing MPTCP endpoints are present; set RESET_ENDPOINTS=1 to replace them'
        ip mptcp endpoint show
    fi
fi

if [[ ${DRY_RUN:-0} != 1 ]]; then
    ip mptcp endpoint delete id "$ENDPOINT_ID" 2>/dev/null || true
fi

run ip mptcp limits set subflows "$SUBFLOWS" add_addr_accepted "$ADD_ADDR_ACCEPTED"

case "$ROLE" in
    server|B|b)
        run ip mptcp endpoint add "$LOCAL1" dev "$IF1" id "$ENDPOINT_ID" signal
        ;;
    client|A|a)
        if [[ $LAMINAR == 1 ]]; then
            run ip mptcp endpoint add "$LOCAL1" dev "$IF1" id "$ENDPOINT_ID" subflow laminar
        else
            run ip mptcp endpoint add "$LOCAL1" dev "$IF1" id "$ENDPOINT_ID" subflow
        fi
        ;;
    *) die 'ROLE must be client/server or A/B' ;;
esac

if command -v sysctl >/dev/null; then
    run sysctl -q -w "net.ipv4.conf.$IF0.rp_filter=2"
    run sysctl -q -w "net.ipv4.conf.$IF1.rp_filter=2"
    run sysctl -q -w net.mptcp.path_manager=kernel || true
fi

if [[ ${DRY_RUN:-0} != 1 ]]; then
    ip mptcp endpoint show
    ip mptcp limits show
    log "connect the client to $PEER0 so cable 0 is the initial subflow"
    log 'verify with: ss -Mani; sudo ss -tani; ip -ts mptcp monitor'
fi
