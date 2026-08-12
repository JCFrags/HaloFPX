#!/usr/bin/env bash
set -euo pipefail
OUT=${1:?usage: sample_usb4.sh OUT_DIR [INTERFACE]}
IFACE=${2:-}
mkdir -p "$OUT"
date --utc +%FT%TZ > "$OUT/timestamp_utc.txt"
cat /proc/sys/kernel/random/boot_id > "$OUT/boot_id.txt" 2>/dev/null || true
{
  for d in /sys/bus/thunderbolt/devices/*; do
    [[ -d "$d" ]] || continue
    echo "[$d]"
    for f in name device_name vendor_name generation rx_speed tx_speed rx_lanes tx_lanes authorized unique_id nvm_version; do
      [[ -r "$d/$f" ]] && printf '%s=%s\n' "$f" "$(cat "$d/$f")"
    done
  done
} > "$OUT/thunderbolt-sysfs.txt"
ip -details -statistics link > "$OUT/ip-link.txt"
ip route show table all > "$OUT/routes.txt"
if [[ -n "$IFACE" ]]; then
  ethtool "$IFACE" > "$OUT/ethtool-$IFACE.txt" 2>&1 || true
  ethtool -k "$IFACE" > "$OUT/offloads-$IFACE.txt" 2>&1 || true
  ethtool -S "$IFACE" > "$OUT/stats-$IFACE.txt" 2>&1 || true
fi
journalctl -k --since '-10 min' --no-pager > "$OUT/kernel-last-10m.txt" 2>&1 || true
( cd "$OUT" && find . -type f ! -name MANIFEST.sha256 -print0 | sort -z | xargs -0 sha256sum > MANIFEST.sha256 )
