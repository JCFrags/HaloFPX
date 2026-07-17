---
section_id: "18"
title: "Exact Machine BOM, BIOS, Firmware, Cabling, and Revisions"
status: "needs-machine-validation"
last_verified: "2026-07-17"
applies_to:
  repositories: []
  software_versions: ["CachyOS Linux 7.1.3-1-cachyos (historical observation)"]
  hardware_revisions: ["Nimo Direct MME3L / NIMO Mini PC board version 1.0"]
related_sections: ["17", "19", "20", "21", "22", "23", "84"]
---

# 18 — Exact Machine BOM, BIOS, Firmware, Cabling, and Revisions

This section is the matched-pair identity gate for HaloFPX. Internet specifications cannot prove two physical machines are equivalent; only same-session inventories, port photographs, cable labels, and firmware records can.

## Current disposition

- **[VERIFIED]** Preserved, redacted July 12 audit reports identify both nodes as Nimo Direct MME3L systems with NIMO Mini PC boards, Ryzen AI MAX+ 395 processors, 128 GiB soldered memory populations, Radeon 8060S/gfx1151 devices, and AMI BIOS 3.05 dated 2025-10-11 [S18-L01, S18-L02]. The reports are historical primary project records, but the raw command-output bundle was not available; S18-E01 remains the measurement gate.
- **[MEASURED]** A same-session SSH inventory on 2026-07-17 confirmed both live hosts as Nimo Direct MME3L / NIMO Mini PC board version 1.0, BIOS 3.05 dated 2025-10-11, Ryzen AI MAX+ 395 stepping 0 with microcode `0xb700037`, Radeon 8060S `1002:1586` revision `c1`, current kernel `7.1.3-1-cachyos`, and Crucial P310 firmware `VACR001` [S18-L03].
- **[OPEN]** Chassis serial-to-host mapping, power-supply identity, cooling assembly, exact USB4 cable SKUs/serials, physical port labels, retimer firmware, EC firmware, and cold-boot enumeration repeatability remain unproven.
- **[RECOMMENDATION]** Do not call the pair “matched” until experiment S18-E01 passes every hard field in [procedures_and_checks.md](procedures_and_checks.md).

## Research split

1. Completed now: inventory schema, primary standards/tool routing, and extraction of scoped July 2026 observations.
2. Required on machines: S18-E01 inventory capture, S18-E02 photo/port/cable map, S18-E03 firmware comparison, S18-E04 cold-boot enumeration repeatability.
3. Contingent decisions: whether mismatched firmware, storage, cabling, cooling, or board revision must be normalized before distributed benchmarks.

See [facts and constraints](facts_and_constraints.md), [procedures](procedures_and_checks.md), [design implications](design_implications.md), [open questions](open_questions.md), and [sources](sources.md).
