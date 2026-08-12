#!/usr/bin/env bash
set -Eeuo pipefail
SCRIPT_DIR=$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd)
source "$SCRIPT_DIR/common.sh"

require_root
require_cmd ip
ROLE=${ROLE:-}
IF0=${IF0:-thunderbolt0}
IF1=${IF1:-thunderbolt1}
MTU=${MTU:-9000}
PREFIX=${PREFIX:-30}
role_addresses
validate_iface "$IF0"
validate_iface "$IF1"

[[ $MTU =~ ^[0-9]+$ ]] || die 'MTU must be numeric'
(( MTU >= 68 && MTU <= 65522 )) || die 'MTU must be in thunderbolt-net range 68..65522'

log "role=$ROLE path0=$IF0/$LOCAL0 path1=$IF1/$LOCAL1 mtu=$MTU"
run ip link set dev "$IF0" mtu "$MTU" up
run ip link set dev "$IF1" mtu "$MTU" up
run ip address replace "$LOCAL0/$PREFIX" dev "$IF0"
run ip address replace "$LOCAL1/$PREFIX" dev "$IF1"

if command -v sysctl >/dev/null; then
    run sysctl -q -w "net.ipv4.conf.$IF0.rp_filter=2"
    run sysctl -q -w "net.ipv4.conf.$IF1.rp_filter=2"
fi

if [[ ${DRY_RUN:-0} != 1 ]]; then
    ip -br address show dev "$IF0"
    ip -br address show dev "$IF1"
    ip route get "$PEER0" from "$LOCAL0"
    ip route get "$PEER1" from "$LOCAL1"
fi
