#!/usr/bin/env bash
# Read-only Linux inventory for USB4/Thunderbolt interdomain topology.
set -u
section() { printf '\n===== %s =====\n' "$1"; }
section "timestamp"
date --iso-8601=seconds 2>/dev/null || date
section "kernel"
uname -a
section "thunderbolt sysfs"
if compgen -G '/sys/bus/thunderbolt/devices/*' >/dev/null; then
  for d in /sys/bus/thunderbolt/devices/*; do
    echo "--- $d"
    for f in device_name vendor_name unique_id generation rx_speed tx_speed authorized security; do
      [ -r "$d/$f" ] && printf '%s=%s\n' "$f" "$(cat "$d/$f" 2>/dev/null)"
    done
    readlink -f "$d/device" 2>/dev/null || true
  done
else
  echo "No /sys/bus/thunderbolt/devices entries found."
fi
section "network interfaces"
if command -v ip >/dev/null 2>&1; then ip -details -statistics link; else ls -l /sys/class/net; fi
section "interface drivers"
for n in /sys/class/net/*; do
  iface=${n##*/}
  echo "--- $iface"
  readlink -f "$n/device/driver" 2>/dev/null || true
  if command -v ethtool >/dev/null 2>&1; then ethtool -i "$iface" 2>/dev/null || true; fi
done
section "boltctl"
if command -v boltctl >/dev/null 2>&1; then boltctl list; else echo "boltctl not installed"; fi
section "PCI/USB controllers"
if command -v lspci >/dev/null 2>&1; then lspci -nnk | grep -iA4 -E 'usb|thunderbolt' || true; fi
if command -v lsusb >/dev/null 2>&1; then lsusb -t || true; fi
section "routes and addresses"
if command -v ip >/dev/null 2>&1; then ip address; ip route; fi
