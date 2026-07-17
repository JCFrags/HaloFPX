#!/usr/bin/env bash
set -euo pipefail

APPLY=0
IFACE="thunderbolt0"
ADDRESS=""
MTU=""

usage() {
    echo "Usage: usb4-net-setup.sh [--apply] --address CIDR [--interface IFACE] [--mtu N]"
}
while [[ $# -gt 0 ]]; do
    case "$1" in
        --apply) APPLY=1; shift ;;
        --interface) IFACE="${2:?missing interface}"; shift 2 ;;
        --address) ADDRESS="${2:?missing CIDR}"; shift 2 ;;
        --mtu) MTU="${2:?missing MTU}"; shift 2 ;;
        -h|--help) usage; exit 0 ;;
        *) echo "Unknown argument: $1" >&2; usage >&2; exit 2 ;;
    esac
done
[[ -n "$ADDRESS" ]] || { usage >&2; exit 2; }
[[ "$ADDRESS" == */* ]] || { echo "Address must include CIDR prefix" >&2; exit 2; }
if [[ -n "$MTU" && ! "$MTU" =~ ^[0-9]+$ ]]; then echo "MTU must be numeric" >&2; exit 2; fi

if [[ $EUID -eq 0 ]]; then SUDO=(); else SUDO=(sudo); fi
cmds=(
    "${SUDO[*]} modprobe thunderbolt-net"
    "${SUDO[*]} ip link set '$IFACE' up"
    "${SUDO[*]} ip address replace '$ADDRESS' dev '$IFACE'"
)
if [[ -n "$MTU" ]]; then cmds+=("${SUDO[*]} ip link set '$IFACE' mtu '$MTU'"); fi

printf 'Planned commands:\n'
printf '  %s\n' "${cmds[@]}"
printf 'No default route will be changed.\n'
if [[ "$APPLY" -ne 1 ]]; then
    echo "Dry run only. Re-run with --apply to execute."
    exit 0
fi

"${SUDO[@]}" modprobe thunderbolt-net
for _ in {1..10}; do ip link show "$IFACE" >/dev/null 2>&1 && break; sleep 0.2; done
ip link show "$IFACE" >/dev/null 2>&1 || { echo "Interface not found after module load: $IFACE" >&2; exit 1; }
"${SUDO[@]}" ip link set "$IFACE" up
"${SUDO[@]}" ip address replace "$ADDRESS" dev "$IFACE"
if [[ -n "$MTU" ]]; then "${SUDO[@]}" ip link set "$IFACE" mtu "$MTU"; fi
ip -details address show dev "$IFACE"
printf 'Rollback address: %s ip address del %q dev %q\n' "${SUDO[*]}" "$ADDRESS" "$IFACE"
