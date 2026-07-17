---
section_id: "21"
title: "NVMe and Storage Topology, Performance, and Endurance"
status: "needs-machine-validation"
last_verified: "2026-07-17"
applies_to:
  repositories: []
  software_versions: ["Linux 7.2 documentation", "fio 3.41"]
  hardware_revisions: ["two project Strix Halo nodes; exact BOM open"]
related_sections: ["18", "19", "22", "28", "60", "65", "73", "77", "80"]
---

# 21 - NVMe and Storage Topology, Performance, and Endurance

## High-value conclusion

**[MEASURED]** The 2026-07-17 live inventory identifies both installed drives as Crucial P310 `CT1000P310SSD8` devices with firmware `VACR001`, Btrfs root/home/cache subvolumes, current capacity/headroom, and SMART health counters [S21-L01]. It does not yet establish PCIe negotiated width, queue policy, sustained performance, thermal throttling, write amplification, or power-loss behavior.

**[MEASURED]** nimo-1 had only about 43 GiB free while retaining an approximately 112 GiB RPC tensor cache; nimo-2 had about 318 GiB free and no corresponding worker cache [S21-L01]. This is an immediate capacity gate for HaloKV and 200–230 GB model staging.

**[VERIFIED]** Linux NVMe I/O uses the multi-queue block layer, whose hardware/software queue topology and selectable scheduler can affect latency and fairness ([S21-03], [S21-04]). NVMe SMART records include temperature, critical warnings, percentage used, data written, unsafe shutdowns, and media errors, but `percentage_used` is a vendor estimate rather than a universal failure threshold ([S21-01], [S21-02]).

## Authoritative pages

- [Facts and constraints](facts_and_constraints.md) - evidence schema, endurance semantics, and footprint formulas.
- [Design implications](design_implications.md) - placement and durability consequences for HaloFPX.
- [Procedures and checks](procedures_and_checks.md) - read-only inventory and bounded file-based tests.
- [Open questions](open_questions.md) - machine evidence and contingent decisions.
- [Sources](sources.md) - primary-source records.

## Research split

1. **Internet research complete:** NVMe health semantics, Linux queue behavior, filesystem durability boundary, and fio controls.
2. **Machine work required:** negotiated PCIe/queue detail plus all performance, endurance-delta, thermal, concurrent-load, and power-interruption results.
3. **Contingent decisions:** cache device, filesystem/mount options, queue policy, reserve capacity, write budget, and durability mode.

[S21-01]: sources.md#s21-01
[S21-02]: sources.md#s21-02
[S21-03]: sources.md#s21-03
[S21-04]: sources.md#s21-04
