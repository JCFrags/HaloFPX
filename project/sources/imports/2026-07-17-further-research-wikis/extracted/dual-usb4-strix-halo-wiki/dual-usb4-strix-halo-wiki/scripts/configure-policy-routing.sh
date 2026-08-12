#!/usr/bin/env bash
set -Eeuo pipefail
SCRIPT_DIR=$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd)
source "$SCRIPT_DIR/common.sh"

require_root
require_cmd ip
ROLE=${ROLE:-}
IF0=${IF0:-thunderbolt0}
IF1=${IF1:-thunderbolt1}
TABLE0=${TABLE0:-144}
TABLE1=${TABLE1:-145}
PRIO0=${PRIO0:-10440}
PRIO1=${PRIO1:-10441}
role_addresses
validate_iface "$IF0"
validate_iface "$IF1"

run ip route replace table "$TABLE0" 10.44.0.0/30 dev "$IF0" src "$LOCAL0"
run ip route replace table "$TABLE1" 10.44.1.0/30 dev "$IF1" src "$LOCAL1"

if [[ ${DRY_RUN:-0} == 1 ]]; then
    run ip rule add priority "$PRIO0" from "$LOCAL0/32" lookup "$TABLE0"
    run ip rule add priority "$PRIO1" from "$LOCAL1/32" lookup "$TABLE1"
else
    while ip rule del priority "$PRIO0" 2>/dev/null; do :; done
    while ip rule del priority "$PRIO1" 2>/dev/null; do :; done
    ip rule add priority "$PRIO0" from "$LOCAL0/32" lookup "$TABLE0"
    ip rule add priority "$PRIO1" from "$LOCAL1/32" lookup "$TABLE1"
fi

if command -v sysctl >/dev/null; then
    run sysctl -q -w net.ipv4.conf.all.rp_filter=2
    run sysctl -q -w "net.ipv4.conf.$IF0.rp_filter=2"
    run sysctl -q -w "net.ipv4.conf.$IF1.rp_filter=2"
fi

if [[ ${DRY_RUN:-0} != 1 ]]; then
    ip -4 rule show
    ip -4 route show table "$TABLE0"
    ip -4 route show table "$TABLE1"
    ip route get "$PEER0" from "$LOCAL0"
    ip route get "$PEER1" from "$LOCAL1"
fi
