---
section_id: "18"
title: "BOM sources"
status: "needs-machine-validation"
last_verified: "2026-07-17"
applies_to:
  repositories: []
  software_versions: []
  hardware_revisions: ["Nimo Direct MME3L / NIMO Mini PC board version 1.0"]
related_sections: ["20", "21", "22", "23"]
---

# Sources

Access date for web sources: 2026-07-16.

## S18-L01 — nimo-1 deep-system audit

- Canonical project copy: [`../../../../sources/measurements/2026-07-10_12-strix-halo-cluster/redacted-audits/2026-07-12__nimo-1__deep-system-audit__v01.md`](../../../../sources/measurements/2026-07-10_12-strix-halo-cluster/redacted-audits/2026-07-12__nimo-1__deep-system-audit__v01.md)
- Original local path: `C:/Users/britt/AgentRoot/10_projects/11_active/11.02_strix-halo-usb4-cluster/01_discovery/output/2026-07-12__nimo-1__deep-system-audit__v01.md`
- SHA-256: `03982946a2eb8fd18d6117861c5e4c75f43986fb366a1da5b57416f5ab2a50f2` (copy matched original on 2026-07-17).
- Revision/date: v01, capture 2026-07-12; independently reviewed according to the artifact.
- Supports: historical nimo-1 DMI/BOM, BIOS, memory, GPU, NVMe, boot, USB4 and firmware-visible state.
- Limitation: synthesized audit with redactions; not a fresh raw capture and not complete for cables/board revision.

## S18-L02 — nimo-2 deep-system audit

- Canonical project copy: [`../../../../sources/measurements/2026-07-10_12-strix-halo-cluster/redacted-audits/2026-07-12__nimo-2__deep-system-audit__v01.md`](../../../../sources/measurements/2026-07-10_12-strix-halo-cluster/redacted-audits/2026-07-12__nimo-2__deep-system-audit__v01.md)
- Original local path: `C:/Users/britt/AgentRoot/10_projects/11_active/11.02_strix-halo-usb4-cluster/01_discovery/output/2026-07-12__nimo-2__deep-system-audit__v01.md`
- SHA-256: `ecdc400942a1ed95615aeaddc83d2c78e2c38a9fcdcc0b56a68a77468b26e410` (copy matched original on 2026-07-17).
- Revision/date: v01, capture 2026-07-12; independently reviewed according to the artifact.
- Supports: historical nimo-2 identity, BOM, software, memory and USB4 comparison.
- Limitation: same as S18-L01.

## S18-01 — DMTF SMBIOS specification

- Publisher: DMTF; [SMBIOS specification landing page](https://www.dmtf.org/standards/smbios)
- Revision: current landing page; exact implemented SMBIOS version must be captured per host.
- Supports: inventory structure semantics.
- Limitation: OEM firmware may omit or populate placeholder values.

## S18-02 — Linux USB4 and Thunderbolt administrator guide

- Publisher: Linux kernel; [kernel 6.16 guide](https://docs.kernel.org/6.16/admin-guide/thunderbolt.html)
- Revision: Linux 6.16 documentation.
- Supports: domains, security, devices, host/retimer NVM, `fwupd`, networking enumeration.
- Limitation: running hosts were historically kernel 7.1.3; verify ABI there.

## S18-03 — fwupd documentation

- Publisher/repository: fwupd project; [fwupd README](https://github.com/fwupd/fwupd)
- Revision: live repository; pin installed package and daemon versions during E03.
- Supports: device and firmware inventory routing.
- Limitation: availability depends on OEM/LVFS metadata; discovery does not authorize updates.

## S18-L03 — Live matched-pair inventory

- Canonical project source: [`../../../../sources/measurements/2026-07-17-strix-halo-live-inventory/README.md`](../../../../sources/measurements/2026-07-17-strix-halo-live-inventory/README.md)
- Capture: SSH to both nodes, 2026-07-17 11:52–12:05 America/Los_Angeles.
- Supports: current DMI, BIOS, CPU/microcode, GPU, OS/kernel, storage, package, USB4, and runtime comparison.
- Limitations: normalized/redacted capture; physical cable/port labels, EC/retimer firmware, power/cooling identity, and cold-boot stability remain open.
