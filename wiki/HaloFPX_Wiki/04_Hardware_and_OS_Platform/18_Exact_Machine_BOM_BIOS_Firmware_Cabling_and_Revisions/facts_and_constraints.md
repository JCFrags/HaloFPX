---
section_id: "18"
title: "BOM facts and matching constraints"
status: "needs-machine-validation"
last_verified: "2026-07-17"
applies_to:
  repositories: []
  software_versions: ["CachyOS Linux 7.1.3-1-cachyos (historical observation)"]
  hardware_revisions: ["Nimo Direct MME3L / NIMO Mini PC board version 1.0"]
related_sections: ["17", "19", "20", "21", "22", "23"]
---

# Facts and constraints

## Scoped observations from the July 12 audits

These are **[VERIFIED]** historical observations reported by preserved, redacted audit artifacts, not a claim about current state [S18-L01, S18-L02]. The underlying raw command-output bundle was not available for preservation, so these rows are not `[MEASURED]`; S18-E01 defines the required reproducible recapture.

| Field | nimo-1 | nimo-2 | Disposition |
|---|---|---|---|
| System / board | Nimo Direct MME3L / NIMO Mini PC | Nimo Direct Inc. MME3L / NIMO Mini PC | Same naming; board revision absent |
| BIOS | AMI 3.05, 2025-10-11 | AMI 3.05, 2025-10-11 | Historically matched |
| CPU | Ryzen AI MAX+ 395, 16C/32T | Ryzen AI MAX+ 395, 16C/32T | Historically matched |
| Memory | 8 × 16 GiB Samsung K3KLALA0EM-MGCV, 8000 MT/s | 8 × 16 GiB Samsung K3KLALA0EM-MGCV, 8000 MT/s | Historically matched; inventory did not prove LPDDR5X suffix |
| GPU | Radeon 8060S, `1002:1586` rev `c1`, gfx1151, 40 CU | Same reported class | Re-capture full PCI tuple on both |
| NVMe | Crucial P310 1 TB, firmware VACR001 | Crucial P310 1 TB, firmware VACR001 | Re-capture serial hashes, namespace, link, SMART |
| OS/kernel | CachyOS, 7.1.3-1-cachyos | Matching at preflight | Volatile; compare package/build IDs |
| USB4 | two Linux domains/netdevs; two retimers per observed path | two Linux domains/netdevs | Controller/retimer IDs and firmware incomplete |
| Boot policy | UEFI; Secure Boot off; `amd_iommu=off`; ASPM off | Equivalent class reported | Must compare exact command line and firmware settings |

## Live matched-pair capture — 2026-07-17

The following are **[MEASURED]** same-session observations from the normalized live source package [S18-L03]:

| Field | nimo-1 | nimo-2 | Disposition |
|---|---|---|---|
| System / board | Nimo Direct MME3L / NIMO Mini PC v1.0 | same | matched through DMI |
| BIOS | AMI 3.05, 2025-10-11 | same | matched |
| CPU | Ryzen AI MAX+ 395, stepping 0, microcode `0xb700037` | same | matched |
| GPU | Radeon 8060S, `1002:1586` rev `c1`, gfx1151, 40 CU | same | matched |
| Current OS/kernel | CachyOS / `7.1.3-1-cachyos` | same | matched at capture |
| NVMe | Crucial P310 1 TB, firmware `VACR001` | same model/firmware | serials differ as expected and are redacted here |
| USB4 host functions | `c7:00.5`, `c7:00.6` | same | netdev/domain numbering differs; see Section 20 |
| Boot-policy differences | no explicit zswap flag; swap priority `-1` | `zswap.enabled=0`; swap priority `100` | not a fully normalized pair |
| Package differences | `hipcub` and aggregate `rocm-hip-sdk` observed | not observed in matched package subset | build environment is not fully normalized |

**[INFERENCE]** The pair is matched closely enough to use as the target BOM, but performance comparisons still require an experiment record that pins boot parameters, package closure, swap policy, temperatures, and workload state.

## What “matched” means

- **[VERIFIED]** SMBIOS is the standard interface used to expose system, baseboard, BIOS, memory-device, power-supply, cooling-device, and related inventory structures [S18-01]. Absence or OEM placeholders are possible, so SMBIOS alone is not sufficient.
- **[VERIFIED]** Linux exposes USB4 domain/device and retimer attributes through the Thunderbolt sysfs ABI; firmware inventory should include those objects and `fwupdmgr get-devices` output [S18-02, S18-03].
- **[RECOMMENDATION]** Hard-match fields: system/board product and revision, CPU stepping/microcode, memory population and speed, BIOS/EC/USB4 firmware, kernel and boot parameters, GPU/controller PCI IDs/revisions, cable capability, and physical port pairing.
- **[RECOMMENDATION]** Controlled-difference fields: NVMe health/wear, serial numbers, MACs, and UUIDs. They must be recorded but are expected to differ.
- **[INFERENCE]** Equal marketing models do not imply equal USB4 roots, retimers, firmware, thermal assemblies, or cable paths; those differences can change transport tails and sustained clocks.

## Inventory record requirements

Each record needs: capture UTC and monotonic start/end, hostname, chassis label, command/tool version, raw-output hash, redacted public summary, and whether captured warm or after cold boot. Retain serials only in restricted evidence; publish salted hashes when identity comparison is needed.
