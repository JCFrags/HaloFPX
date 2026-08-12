#!/usr/bin/env bash
set -euo pipefail
OUT=${1:?usage: collect_provenance.sh OUT_DIR [MODEL_FILE] [RUNTIME_BINARY]}
MODEL=${2:-}
RUNTIME=${3:-}
mkdir -p "$OUT"
run() { local name=$1; shift; { printf '$'; printf ' %q' "$@"; printf '\n'; "$@"; } >"$OUT/$name.txt" 2>&1 || true; }

printf '%s\n' "$(date --utc +%FT%TZ)" > "$OUT/collected_at_utc.txt"
printf '%s\n' "$(cat /proc/sys/kernel/random/boot_id 2>/dev/null || true)" > "$OUT/boot_id.txt"
run uname uname -a
run os-release cat /etc/os-release
run cmdline cat /proc/cmdline
run lscpu lscpu --all --extended
run lscpu-json lscpu --json
run lspci lspci -nnk
run lsusb lsusb -tv
run memory cat /proc/meminfo
run mounts findmnt --json
run block lsblk --json --bytes --output-all
run network ip -details -statistics address
run routes ip -details route show table all
run listeners ss -lntup
run modules lsmod
run modinfo-amdgpu modinfo amdgpu
run packages-dpkg dpkg-query -W '-f=${binary:Package}\t${Version}\n'
run packages-rpm rpm -qa
run rocminfo rocminfo
run amd-smi amd-smi static --all
run rocm-smi rocm-smi --showallinfo
run hipcc hipcc --version
run clang clang --version
run gcc gcc --version
run cmake cmake --version
run python python3 --version
run iperf3 iperf3 --version
run ethtool ethtool --version

# Text-safe USB4/Thunderbolt attributes.
{
  for d in /sys/bus/thunderbolt/devices/*; do
    [[ -d "$d" ]] || continue
    echo "[$d]"
    for f in name device_name vendor_name generation rx_speed tx_speed rx_lanes tx_lanes authorized unique_id; do
      [[ -r "$d/$f" ]] && printf '%s=%s\n' "$f" "$(cat "$d/$f")"
    done
  done
} > "$OUT/usb4-sysfs.txt" 2>&1 || true

{
  for d in /sys/class/drm/card*/device; do
    [[ -d "$d" ]] || continue
    echo "[$d]"
    for f in vendor device revision subsystem_vendor subsystem_device gpu_busy_percent mem_busy_percent mem_info_vram_total mem_info_vram_used; do
      [[ -r "$d/$f" ]] && printf '%s=%s\n' "$f" "$(cat "$d/$f")"
    done
  done
} > "$OUT/amdgpu-sysfs.txt" 2>&1 || true

[[ -n "$MODEL" && -f "$MODEL" ]] && { sha256sum "$MODEL" > "$OUT/model.sha256"; stat --printf='%n\nbytes=%s\nallocated_blocks=%b\nblock_size=%B\n' "$MODEL" > "$OUT/model.stat"; }
[[ -n "$RUNTIME" && -f "$RUNTIME" ]] && { sha256sum "$RUNTIME" > "$OUT/runtime.sha256"; run runtime-ldd ldd "$RUNTIME"; }

# Never capture the full environment: it may contain API keys and credentials.
( cd "$OUT" && find . -type f ! -name MANIFEST.sha256 -print0 | sort -z | xargs -0 sha256sum > MANIFEST.sha256 )
printf 'Provenance written to %s\n' "$OUT"
