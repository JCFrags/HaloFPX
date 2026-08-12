---
section_id: "18"
title: "BOM inventory procedures and checks"
status: "needs-machine-validation"
last_verified: "2026-07-16"
applies_to:
  repositories: []
  software_versions: ["Linux"]
  hardware_revisions: ["Nimo Direct MME3L / NIMO Mini PC (revision unknown)"]
related_sections: ["20", "21", "22", "23", "84"]
---

# Procedures and checks

Run on both machines within one maintenance window. Commands are read-only unless marked. Root is needed for complete DMI and some firmware/kernel logs. Save raw output, stderr, exit status, tool version, UTC timestamps, and SHA-256.

## S18-E01 — Canonical matched-pair inventory

```bash
hostnamectl
uname -a; cat /proc/cmdline
sudo dmidecode --type 0,1,2,3,4,16,17,27,39
lscpu -e; grep -m1 microcode /proc/cpuinfo
sudo lspci -Dnnvvk
lsmem; free -b; numactl --hardware
rocminfo; rocm-smi --showproductname --showuniqueid --showmeminfo all
lsblk -b -O; sudo nvme list; sudo nvme smart-log /dev/nvme0
fwupdmgr get-devices --json
find /sys/bus/thunderbolt/devices -maxdepth 3 -type f -readable -print -exec cat {} \;
find /lib/firmware/amdgpu -maxdepth 1 -type f -printf '%f\n' | sort
sha256sum /boot/vmlinuz-* 2>/dev/null
```

Compare normalized JSON/TSV, not terminal text. Remove serial values only after computing restricted hashes.

## S18-E02 — Physical port and cable map

1. Power down both hosts. Photograph chassis labels, PSU labels, cooling vents, every USB-C port label, and both cable markings/ends.
2. Assign durable labels `N1-P1`, `N1-P2`, `N2-P1`, `N2-P2`, `CABLE-A`, `CABLE-B`.
3. Boot with one cable at a time and map the resulting domain, XDomain UUID hash, netdev MAC hash, PCI root/NHI, retimer chain, and negotiated lanes/speed.
4. Swap cables while holding ports fixed, then swap ports while holding cables fixed. This separates cable and port effects.

## S18-E03 — Firmware comparison

Capture BIOS setup screenshots/export where supported, `fwupdmgr get-devices --json`, CPU microcode, amdgpu firmware filenames/hashes, NVMe firmware, USB4 host-router and retimer NVM versions, EC version, Secure Boot, IOMMU, ASPM, VRAM carve-out, power mode, and fan mode. **Do not update firmware during discovery.**

## S18-E04 — Enumeration stability

Repeat E01 after three cold boots and after cable-order reversal. Pass if durable identities and physical mappings remain consistent even when domain/netdev numbers change. Record any instability.

## Pair verdict

`PASS`: all hard fields present and equal; expected-unique fields differ only by identity; physical mapping stable. `CONDITIONAL`: documented controlled difference with evidence it does not affect the experiment. `FAIL`: missing hard evidence, mismatched firmware/revision/settings, or unstable port mapping.
