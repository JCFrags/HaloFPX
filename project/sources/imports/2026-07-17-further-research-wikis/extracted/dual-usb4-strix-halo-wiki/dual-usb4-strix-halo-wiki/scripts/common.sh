#!/usr/bin/env bash
# Shared helpers. Source from scripts; do not execute directly.

log() { printf '[dual-usb4] %s\n' "$*" >&2; }
warn() { printf '[dual-usb4] WARNING: %s\n' "$*" >&2; }
die() { printf '[dual-usb4] ERROR: %s\n' "$*" >&2; exit 1; }

require_root() {
    [[ ${EUID:-$(id -u)} -eq 0 ]] || die 'run as root (sudo)'
}

require_cmd() {
    command -v "$1" >/dev/null 2>&1 || die "required command not found: $1"
}

run() {
    if [[ ${DRY_RUN:-0} == 1 ]]; then
        printf '+ ' >&2
        printf '%q ' "$@" >&2
        printf '\n' >&2
    else
        "$@"
    fi
}

role_addresses() {
    case "${ROLE:-}" in
        A|a|client)
            LOCAL0=${LOCAL0:-10.44.0.1}
            PEER0=${PEER0:-10.44.0.2}
            LOCAL1=${LOCAL1:-10.44.1.1}
            PEER1=${PEER1:-10.44.1.2}
            ;;
        B|b|server)
            LOCAL0=${LOCAL0:-10.44.0.2}
            PEER0=${PEER0:-10.44.0.1}
            LOCAL1=${LOCAL1:-10.44.1.2}
            PEER1=${PEER1:-10.44.1.1}
            ;;
        *) die 'ROLE must be A/B or client/server' ;;
    esac
    export LOCAL0 PEER0 LOCAL1 PEER1
}

validate_iface() {
    local i=$1
    [[ -d /sys/class/net/$i ]] || die "network interface not found: $i"
}
