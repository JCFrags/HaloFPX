#!/usr/bin/env bash
set -euo pipefail
OUT="${1:-$PWD/usb4stream-preflight-$(date -u +%Y%m%dT%H%M%SZ)}"
mkdir -p "$OUT"

{
  echo "captured_at_utc=$(date -u +%FT%TZ)"
  echo "kernel=$(uname -r)"
  echo "hostname=$(hostname -f 2>/dev/null || hostname)"
} > "$OUT/identity.txt"

config=""
if [[ -r /proc/config.gz ]]; then
  zcat /proc/config.gz > "$OUT/kernel-config.txt"; config="$OUT/kernel-config.txt"
elif [[ -r "/boot/config-$(uname -r)" ]]; then
  cp "/boot/config-$(uname -r)" "$OUT/kernel-config.txt"; config="$OUT/kernel-config.txt"
fi
[[ -n "$config" ]] || { echo 'Kernel config unavailable' >&2; exit 2; }

grep -E '^CONFIG_USB4=y|^CONFIG_USB4=m' "$config" > "$OUT/config-usb4.txt" || { echo 'CONFIG_USB4 missing' >&2; exit 3; }
grep -E '^CONFIG_USB4_CONFIGFS=y' "$config" > "$OUT/config-configfs.txt" || { echo 'CONFIG_USB4_CONFIGFS missing' >&2; exit 3; }
grep -E '^CONFIG_USB4_STREAM=y|^CONFIG_USB4_STREAM=m' "$config" > "$OUT/config-stream.txt" || { echo 'CONFIG_USB4_STREAM missing' >&2; exit 3; }

grep -E '^CONFIG_(IOMMU_SUPPORT|AMD_IOMMU)=y' "$config" > "$OUT/config-iommu.txt" || { echo 'IOMMU protection config not confirmed' >&2; exit 4; }
modinfo thunderbolt-stream > "$OUT/thunderbolt-stream-modinfo.txt" 2>&1 || { echo 'thunderbolt-stream module unavailable' >&2; exit 5; }
ls -l /sys/kernel/config > "$OUT/configfs-root.txt" 2>&1 || true
ls -l /sys/kernel/config/thunderbolt/stream > "$OUT/stream-configfs.txt" 2>&1 || true
find /sys/bus/thunderbolt -maxdepth 4 -print > "$OUT/thunderbolt-sysfs.txt" 2>&1 || true
find /dev -maxdepth 1 -name 'tbstream*' -ls > "$OUT/tbstream-devices.txt" 2>&1 || true
dmesg --color=never | grep -Ei 'thunderbolt|USB4|IOMMU|AMD-Vi|amdgpu|amdkfd|kfd' > "$OUT/dmesg-relevant.txt" 2>&1 || true

cat > "$OUT/QUALIFICATION-STATUS.txt" <<'EOF'
[UNVERIFIED_COMBINATION]
This preflight checks presence/configuration only. It does not qualify USB4STREAM,
ROCm, gfx1151, hotplug, suspend/resume, DMA protection, throughput or two-node behavior.
EOF
echo "USB4STREAM preflight recorded in $OUT"
