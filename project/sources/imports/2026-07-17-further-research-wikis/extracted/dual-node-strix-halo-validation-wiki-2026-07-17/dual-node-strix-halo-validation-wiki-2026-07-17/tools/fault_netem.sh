#!/usr/bin/env bash
# Guarded helper for a dedicated TEST interface only. Never point this at management.
set -euo pipefail
ACTION=${1:?usage: CONFIRM_FAULT_INJECTION=YES fault_netem.sh apply|clear|status IFACE [netem args...]}
IFACE=${2:?}; shift 2
[[ ${CONFIRM_FAULT_INJECTION:-NO} == YES ]] || { echo "set CONFIRM_FAULT_INJECTION=YES" >&2; exit 3; }
[[ -d /sys/class/net/$IFACE ]] || { echo "interface not found: $IFACE" >&2; exit 4; }
[[ ${MANAGEMENT_INTERFACE:-} != "$IFACE" ]] || { echo "refusing management interface" >&2; exit 5; }
case "$ACTION" in
  apply)
    [[ $# -gt 0 ]] || { echo "supply netem args, e.g. delay 50ms 5ms" >&2; exit 2; }
    sudo -n tc qdisc replace dev "$IFACE" root netem "$@"
    ;;
  clear) sudo -n tc qdisc del dev "$IFACE" root 2>/dev/null || true ;;
  status) tc -s qdisc show dev "$IFACE" ;;
  *) exit 2 ;;
esac
