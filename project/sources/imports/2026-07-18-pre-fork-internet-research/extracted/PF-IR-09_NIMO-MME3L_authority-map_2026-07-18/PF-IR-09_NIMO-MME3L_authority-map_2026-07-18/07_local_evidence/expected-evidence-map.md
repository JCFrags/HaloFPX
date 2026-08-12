# Expected evidence map

| Gap | Primary collector outputs |
|---|---|
| Exact MME3L / board v1.0 / BIOS | `commands/dmidecode-system.txt`, `sysfs/dmi-id/` |
| PCI 1002:1586 rev c1 mapping | `commands/lspci-nnvv.txt`, `lspci-nnk.txt`, `sysfs/pci-devices/` |
| CPUID and kernel floor | `commands/lscpu*.txt`, `uname.txt`, package inventory |
| AMDGPU IP/firmware tuple | `commands/amdgpu-ip-discovery.txt`, `amdgpu-module-firmware.txt`, firmware hashes, kernel logs |
| AMDGPU RAS | `commands/amdgpu-ras-mask.txt`, `amdgpu-ras-sysfs.txt`, `sysfs/drm/` |
| EDAC/MCA | `sysfs/edac/`, `sysfs/machinecheck/`, `journal-ras-filter.txt` |
| PCIe AER/DPC | `lspci-nnvv.txt`, `aer-dpc-counters.txt`, `_OSC`/AER log entries |
| Crucial P310/VACR001 | `nvme-list*.txt`, `nvme-id-ctrl-*`, `nvme-fw-log-*`, `nvme-smart-*`, `nvme-error-*`, `smartctl-*` |
| LVFS/fwupd GUID and rollback policy | `fwupd-devices-json.txt`, updates/history/remotes/security/config |
| USB4 exact identities/NVM/security | `boltctl-*`, `sysfs/thunderbolt/`, `lspci/lsusb`, IOMMU groups/logs |
| Secure Boot / platform trust | `mokutil-*`, `bootctl.txt`, `fwupd-security*.txt`, TPM/IMA evidence collected separately if deployed |
