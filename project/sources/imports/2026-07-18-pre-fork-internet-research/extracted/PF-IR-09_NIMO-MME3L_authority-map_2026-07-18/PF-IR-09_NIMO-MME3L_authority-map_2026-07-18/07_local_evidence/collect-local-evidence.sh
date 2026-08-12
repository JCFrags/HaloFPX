#!/usr/bin/env bash
# PF-IR-09 read-only local evidence collector.
# It performs no firmware writes, no device resets, no stress tests, no metadata refresh,
# and no configuration changes. Some commands need root to expose complete read-only data.
set -uo pipefail

STAMP="$(date -u +%Y%m%dT%H%M%SZ)"
OUT="${1:-PF-IR-09-local-evidence-${STAMP}}"
umask 077
mkdir -p "$OUT"/{commands,sysfs,logs,firmware,nvme,fwupd,usb4,ras,packages}

log="$OUT/collector.log"
exec 3>>"$log"
printf 'PF-IR-09 collector start: %s\nOutput: %s\n' "$(date -u --iso-8601=seconds)" "$OUT" >&3

have() { command -v "$1" >/dev/null 2>&1; }
run() {
  local name="$1"; shift
  {
    printf '# UTC: %s\n' "$(date -u --iso-8601=seconds)"
    printf '# Command:'; printf ' %q' "$@"; printf '\n'
    "$@"
    rc=$?
    printf '\n# Exit: %s\n' "$rc"
    exit "$rc"
  } >"$OUT/commands/${name}.txt" 2>&1 || true
}
run_sh() {
  local name="$1" script="$2"
  {
    printf '# UTC: %s\n# Shell: %s\n' "$(date -u --iso-8601=seconds)" "$script"
    bash -o pipefail -c "$script"
    rc=$?
    printf '\n# Exit: %s\n' "$rc"
    exit "$rc"
  } >"$OUT/commands/${name}.txt" 2>&1 || true
}
copy_text_tree() {
  local src="$1" dst="$2" maxdepth="${3:-5}"
  mkdir -p "$dst"
  if [[ -d "$src" ]]; then
    while IFS= read -r -d '' f; do
      rel="${f#$src/}"
      mkdir -p "$dst/$(dirname "$rel")"
      timeout 2 cat "$f" >"$dst/$rel" 2>/dev/null || true
    done < <(find "$src" -maxdepth "$maxdepth" -type f -size -1M -print0 2>/dev/null)
  fi
}

cat >"$OUT/README.txt" <<'TXT'
PF-IR-09 local evidence bundle.
This directory may contain serial numbers, UUIDs, hostnames, Secure Boot state, PCI topology,
firmware versions, kernel logs, and other sensitive inventory. Review before external sharing.
The collector is intended to be read-only. It does not flash firmware, write sysfs controls,
refresh network metadata, reset devices, inject faults, or run destructive diagnostics.
TXT

# Baseline identity and boot state.
run date date -u --iso-8601=seconds
run uname uname -a
run hostnamectl hostnamectl
run os-release cat /etc/os-release
run proc-cmdline cat /proc/cmdline
run lscpu lscpu -J
run lscpu-text lscpu
run kernel-config sh -c 'zcat /proc/config.gz 2>/dev/null || cat /boot/config-$(uname -r) 2>/dev/null'
run modules lsmod
run modinfo-amdgpu modinfo amdgpu
run bootctl bootctl status
run mokutil-sb mokutil --sb-state
run mokutil-pk mokutil --pk
run mokutil-kek mokutil --kek
run mokutil-dbx mokutil --dbx
run systemd-analyze-security systemd-analyze security

# DMI/SMBIOS. Exact MME3L / board / BIOS strings are required.
run dmidecode-system dmidecode -t 0,1,2,3,4,11,16,17,41
copy_text_tree /sys/class/dmi/id "$OUT/sysfs/dmi-id" 2

# PCI/USB topology and revision/subsystem IDs.
run lspci-nnvv lspci -Dnnvv
run lspci-nnk lspci -Dnnk
run lspci-tree lspci -Dtv
run lsusb lsusb
run lsusb-tree lsusb -t
run lsusb-verbose lsusb -v
run udev-pci udevadm info --export-db
copy_text_tree /sys/bus/pci/devices "$OUT/sysfs/pci-devices" 2

# ACPI/IOMMU ownership evidence. acpidump only reads tables when available.
run acpidump acpidump
run_sh iommu-groups 'for g in /sys/kernel/iommu_groups/*; do [[ -e "$g" ]] || continue; echo "GROUP ${g##*/}"; find "$g/devices" -maxdepth 1 -type l -printf "%f -> %l\n"; done'

# fwupd/LVFS inventory. Deliberately no `refresh`, `update`, or `install` action.
run fwupd-version fwupdmgr --version
run fwupd-devices-json fwupdmgr get-devices --json
run fwupd-devices fwupdmgr get-devices
run fwupd-updates-json fwupdmgr get-updates --json
run fwupd-updates fwupdmgr get-updates
run fwupd-history-json fwupdmgr get-history --json
run fwupd-history fwupdmgr get-history
run fwupd-remotes-json fwupdmgr get-remotes --json
run fwupd-remotes fwupdmgr get-remotes
run fwupd-security-json fwupdmgr security --json
run fwupd-security fwupdmgr security
run fwupd-config fwupdmgr get-config

# NVMe inventory and logs. All commands are read-only.
run nvme-version nvme version
run nvme-list-json nvme list -o json
run nvme-list nvme list
run nvme-list-subsys-json nvme list-subsys -o json
run nvme-list-subsys nvme list-subsys
if have nvme; then
  for ctl in /dev/nvme[0-9]*; do
    [[ -e "$ctl" && "$ctl" =~ ^/dev/nvme[0-9]+$ ]] || continue
    base="$(basename "$ctl")"
    run "nvme-id-ctrl-${base}" nvme id-ctrl "$ctl" -H -o json
    run "nvme-fw-log-${base}" nvme fw-log "$ctl" -o json
    run "nvme-smart-${base}" nvme smart-log "$ctl" -H -o json
    run "nvme-error-${base}" nvme error-log "$ctl" -e 256 -o json
    run "udev-${base}" udevadm info --query=all --name="$ctl"
  done
fi
if have smartctl; then
  for dev in /dev/nvme[0-9]* /dev/nvme[0-9]*n[0-9]*; do
    [[ -e "$dev" ]] || continue
    run "smartctl-$(basename "$dev")" smartctl -x "$dev"
  done
fi
copy_text_tree /sys/class/nvme "$OUT/sysfs/nvme" 5

# EDAC/MCA/RAS capability and counters.
copy_text_tree /sys/devices/system/edac "$OUT/sysfs/edac" 8
copy_text_tree /sys/devices/system/machinecheck "$OUT/sysfs/machinecheck" 5
copy_text_tree /sys/devices/system/cpu/cpu0/microcode "$OUT/sysfs/cpu0-microcode" 3
run_sh edac-modules 'lsmod | grep -E "(^|_)(edac|amd64_edac|ghes|apei)" || true'
run_sh ras-sysfs-summary 'find /sys/devices/system/edac /sys/devices/system/machinecheck -type f -maxdepth 8 -print -exec sh -c "cat \"$1\" 2>/dev/null" sh {} \; 2>/dev/null'
run amdgpu-ras-mask cat /sys/module/amdgpu/parameters/ras_mask
run_sh amdgpu-ras-sysfs 'for d in /sys/class/drm/card*/device/ras; do [[ -d "$d" ]] || continue; echo "## $d"; find "$d" -maxdepth 1 -type f -print -exec cat {} \; 2>/dev/null; done'
run_sh amdgpu-ip-discovery 'for f in /sys/kernel/debug/dri/*/amdgpu_ip_info /sys/kernel/debug/dri/*/amdgpu_firmware_info /sys/kernel/debug/dri/*/amdgpu_pm_info /sys/kernel/debug/dri/*/name; do [[ -r "$f" ]] || continue; echo "## $f"; cat "$f"; done'
copy_text_tree /sys/class/drm "$OUT/sysfs/drm" 5

# AER/DPC/link counters in a compact form.
run_sh aer-dpc-counters 'for d in /sys/bus/pci/devices/*; do found=0; for f in "$d"/aer_* "$d"/dpc_* "$d"/current_link_* "$d"/max_link_*; do [[ -f "$f" ]] || continue; if [[ $found -eq 0 ]]; then echo "## ${d##*/}"; found=1; fi; printf "%s=" "${f##*/}"; cat "$f" 2>/dev/null; done; done'

# USB4 / Thunderbolt identity, security, NVM, and topology. Exclude binary nvmem.
run boltctl-list boltctl list --all
run boltctl-domains boltctl domains
run thunderboltctl thunderboltctl list
if [[ -d /sys/bus/thunderbolt/devices ]]; then
  mkdir -p "$OUT/sysfs/thunderbolt"
  while IFS= read -r -d '' f; do
    case "$f" in */nvm_active*/*|*/nvm_non_active*/*|*/nvmem) continue;; esac
    rel="${f#/sys/bus/thunderbolt/devices/}"
    mkdir -p "$OUT/sysfs/thunderbolt/$(dirname "$rel")"
    timeout 2 cat "$f" >"$OUT/sysfs/thunderbolt/$rel" 2>/dev/null || true
  done < <(find /sys/bus/thunderbolt/devices -maxdepth 7 -type f -size -1M -print0 2>/dev/null)
fi

# Kernel and persistent logs. Reading the prior boot may fail or be unavailable.
run dmesg dmesg --color=never --ctime
run journal-kernel-current journalctl -b -k --no-pager -o short-iso-precise
run journal-kernel-previous journalctl -b -1 -k --no-pager -o short-iso-precise
run_sh journal-ras-filter 'journalctl -b -k --no-pager -o short-iso-precise | grep -Ei "mce|machine.check|edac|ecc|aer|dpc|pcie.*error|amdgpu|ras|poison|nvme|thunderbolt|usb4|iommu|_osc|firmware|microcode" || true'
run pstore-list sh -c 'find /sys/fs/pstore -maxdepth 1 -type f -print -exec cat {} \; 2>/dev/null'

# Firmware/package provenance.
run amdgpu-module-firmware modinfo -F firmware amdgpu
run_sh amdgpu-firmware-hashes 'modinfo -F firmware amdgpu 2>/dev/null | sort -u | while read -r rel; do for root in /lib/firmware /usr/lib/firmware; do f="$root/$rel"; [[ -f "$f" ]] && sha256sum "$f"; [[ -f "$f.zst" ]] && sha256sum "$f.zst"; [[ -f "$f.xz" ]] && sha256sum "$f.xz"; done; done'
run dpkg-packages sh -c 'dpkg-query -W -f="${Package}\t${Version}\n" 2>/dev/null | grep -Ei "linux-image|linux-firmware|fwupd|amd|mesa|libdrm|nvme|smartmontools|rasdaemon|bolt"'
run rpm-packages sh -c 'rpm -qa --qf "%{NAME}\t%{VERSION}-%{RELEASE}.%{ARCH}\n" 2>/dev/null | grep -Ei "kernel|linux-firmware|fwupd|amd|mesa|libdrm|nvme|smartmontools|rasdaemon|bolt"'
run pacman-packages sh -c 'pacman -Q 2>/dev/null | grep -Ei "linux|firmware|fwupd|amd|mesa|libdrm|nvme|smartmontools|rasdaemon|bolt"'

# Copy command outputs into topical folders for convenience while retaining canonical commands/.
cp "$OUT"/commands/fwupd-* "$OUT/fwupd/" 2>/dev/null || true
cp "$OUT"/commands/nvme-* "$OUT/nvme/" 2>/dev/null || true
cp "$OUT"/commands/smartctl-* "$OUT/nvme/" 2>/dev/null || true
cp "$OUT"/commands/boltctl-* "$OUT/usb4/" 2>/dev/null || true
cp "$OUT"/commands/thunderboltctl* "$OUT/usb4/" 2>/dev/null || true
cp "$OUT"/commands/*ras* "$OUT/ras/" 2>/dev/null || true
cp "$OUT"/commands/*aer* "$OUT/ras/" 2>/dev/null || true
cp "$OUT"/commands/*edac* "$OUT/ras/" 2>/dev/null || true
cp "$OUT"/commands/*mce* "$OUT/ras/" 2>/dev/null || true
cp "$OUT"/commands/*firmware* "$OUT/firmware/" 2>/dev/null || true
cp "$OUT"/commands/*packages* "$OUT/packages/" 2>/dev/null || true
cp "$OUT"/commands/journal-* "$OUT/logs/" 2>/dev/null || true
cp "$OUT"/commands/dmesg* "$OUT/logs/" 2>/dev/null || true

# Integrity manifest. The manifest excludes itself.
(
  cd "$OUT"
  find . -type f ! -name files.sha256 -print0 | sort -z | xargs -0 sha256sum > files.sha256
)
printf 'PF-IR-09 collector complete: %s\n' "$(date -u --iso-8601=seconds)" >&3
printf 'Evidence directory: %s\n' "$OUT"
printf 'Review for sensitive inventory before sharing. Verify with: (cd %q && sha256sum -c files.sha256)\n' "$OUT"
