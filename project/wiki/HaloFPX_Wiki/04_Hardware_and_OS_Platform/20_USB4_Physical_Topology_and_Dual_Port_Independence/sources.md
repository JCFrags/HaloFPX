---
section_id: "20"
title: "USB4 topology sources"
status: "needs-machine-validation"
last_verified: "2026-07-17"
applies_to:
  repositories: []
  software_versions: ["Linux USB4/Thunderbolt", "thunderbolt-net"]
  hardware_revisions: ["Nimo Direct MME3L / NIMO Mini PC (revision unknown)"]
related_sections: ["18", "19", "23", "50", "55"]
---

# Sources

Web sources accessed 2026-07-16.

## S20-01 — Linux USB4 and Thunderbolt administrator guide

- Publisher: Linux kernel; [current guide](https://docs.kernel.org/admin-guide/thunderbolt.html) and [Linux 6.16 snapshot](https://docs.kernel.org/6.16/admin-guide/thunderbolt.html).
- Supports: routed topology, connection manager, domains, security, IOMMU DMA protection, retimers, host-to-host netdevs, USB4STREAM.
- Limitation: exact ABI/features depend on running kernel/config.

## S20-02 — Linux Thunderbolt sysfs ABI

- Publisher/repository: Linux kernel; [`Documentation/ABI/testing/sysfs-bus-thunderbolt`](https://github.com/torvalds/linux/blob/master/Documentation/ABI/testing/sysfs-bus-thunderbolt)
- Revision: live master; pin selected kernel commit during experiment.
- Supports: domain/router/port/retimer attributes and semantics.
- Limitation: testing ABI can evolve.

## S20-03 — USB4 specification and compliance

- Publisher: USB Implementers Forum; [USB4 specification portal](https://www.usb.org/usb4) and [USB4 compliance](https://www.usb.org/usb4compliance).
- Revision: portal current 2026-07-16; capture exact PDF revision used in future design work.
- Supports: routed/tunneled architecture, link generations and certified cable distinction.
- Limitation: platform implementation and cable certification remain machine-specific.

## Local observations

- **S20-L01:** preserved [nimo-1 audit](../../../../sources/measurements/2026-07-10_12-strix-halo-cluster/redacted-audits/2026-07-12__nimo-1__deep-system-audit__v01.md), SHA-256 `03982946a2eb8fd18d6117861c5e4c75f43986fb366a1da5b57416f5ab2a50f2`; historical topology, lanes, retimers, security, PCI functions and IRQs. Synthesized/redacted, not raw current state.
- **S20-L02:** preserved [nimo-2 audit](../../../../sources/measurements/2026-07-10_12-strix-halo-cluster/redacted-audits/2026-07-12__nimo-2__deep-system-audit__v01.md), SHA-256 `ecdc400942a1ed95615aeaddc83d2c78e2c38a9fcdcc0b56a68a77468b26e410`; historical peer comparison. Same limitations.
- **S20-L03:** preserved [receipt and evidence bundle](../../../../sources/measurements/2026-07-10_12-strix-halo-cluster/README.md), including the [checksum receipt](../../../../sources/measurements/2026-07-10_12-strix-halo-cluster/validation/validation-receipt.md), timestamped [environment/state report](../../../../sources/measurements/2026-07-10_12-strix-halo-cluster/validation/final-report.txt), and raw [`iperf3-links-after.log`](../../../../sources/measurements/2026-07-10_12-strix-halo-cluster/validation/iperf3-links-after.log), [`iperf3-mptcp-socket.log`](../../../../sources/measurements/2026-07-10_12-strix-halo-cluster/validation/iperf3-mptcp-socket.log), and [`iperf3-primary.log`](../../../../sources/measurements/2026-07-10_12-strix-halo-cluster/validation/iperf3-primary.log). Dated 2026-07-10; supports functional two-rail/MPTCP traffic only, not physical independence.
## S20-L04 — Live dual-rail inventory

- Canonical source: [`../../../../sources/measurements/2026-07-17-strix-halo-live-inventory/README.md`](../../../../sources/measurements/2026-07-17-strix-halo-live-inventory/README.md)
- Capture: both nodes, 2026-07-17 11:52–12:05 America/Los_Angeles.
- Supports: current domain/netdev/PCI cross-map, lane/speed/MTU reports, addresses, MPTCP endpoints and two-subflow state, counters, and bounded ICMP RTT diagnostics.
- Limitations: no port/cable labels, simultaneous throughput experiment, controlled counter deltas, or cable/failure test.
