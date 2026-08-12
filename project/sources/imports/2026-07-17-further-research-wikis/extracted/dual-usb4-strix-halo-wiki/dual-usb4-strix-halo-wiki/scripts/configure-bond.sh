#!/usr/bin/env bash
set -Eeuo pipefail
SCRIPT_DIR=$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd)
source "$SCRIPT_DIR/common.sh"

require_root
require_cmd ip
[[ ${CONFIRM:-NO} == YES || ${DRY_RUN:-0} == 1 ]] || die 'bond setup removes member addresses; set CONFIRM=YES or DRY_RUN=1'

ROLE=${ROLE:-}
IF0=${IF0:-thunderbolt0}
IF1=${IF1:-thunderbolt1}
BOND=${BOND:-bond0}
MODE=${MODE:-active-backup}
MTU=${MTU:-9000}
MIIMON=${MIIMON:-100}
XMIT_HASH_POLICY=${XMIT_HASH_POLICY:-layer3+4}
BOND_LOCAL_A=${BOND_LOCAL_A:-10.44.2.1}
BOND_LOCAL_B=${BOND_LOCAL_B:-10.44.2.2}
validate_iface "$IF0"
validate_iface "$IF1"

case "$ROLE" in
    A|a|client) BOND_LOCAL=$BOND_LOCAL_A ;;
    B|b|server) BOND_LOCAL=$BOND_LOCAL_B ;;
    *) die 'ROLE must be A/B or client/server' ;;
esac
case "$MODE" in active-backup|balance-xor|802.3ad|balance-rr) ;; *) die "unsupported mode: $MODE" ;; esac

run modprobe bonding
if [[ ! -d /sys/class/net/$BOND ]]; then
    run ip link add "$BOND" type bond mode "$MODE" miimon "$MIIMON"
fi
run ip link set dev "$BOND" down
run ip link set dev "$IF0" down
run ip link set dev "$IF1" down
run ip address flush dev "$IF0"
run ip address flush dev "$IF1"
run ip link set dev "$IF0" nomaster || true
run ip link set dev "$IF1" nomaster || true

if [[ ${DRY_RUN:-0} != 1 ]]; then
    echo "$MODE" > "/sys/class/net/$BOND/bonding/mode"
    echo "$MIIMON" > "/sys/class/net/$BOND/bonding/miimon"
    if [[ $MODE == balance-xor || $MODE == 802.3ad ]]; then
        echo "$XMIT_HASH_POLICY" > "/sys/class/net/$BOND/bonding/xmit_hash_policy"
    fi
    if [[ $MODE == 802.3ad ]]; then
        echo fast > "/sys/class/net/$BOND/bonding/lacp_rate"
    fi
fi

run ip link set dev "$BOND" mtu "$MTU"
run ip link set dev "$IF0" mtu "$MTU" master "$BOND"
run ip link set dev "$IF1" mtu "$MTU" master "$BOND"
run ip link set dev "$IF0" up
run ip link set dev "$IF1" up
run ip address replace "$BOND_LOCAL/30" dev "$BOND"
run ip link set dev "$BOND" up

if [[ $MODE == active-backup && ${DRY_RUN:-0} != 1 ]]; then
    echo "$IF0" > "/sys/class/net/$BOND/bonding/primary"
fi

if [[ ${DRY_RUN:-0} != 1 ]]; then
    cat "/proc/net/bonding/$BOND"
    warn 'hash modes do not aggregate one TCP connection; balance-rr can reorder packets'
fi
