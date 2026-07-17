---
section_id: "20"
title: "USB4 topology facts and constraints"
status: "needs-machine-validation"
last_verified: "2026-07-17"
applies_to:
  repositories: []
  software_versions: ["Linux USB4/Thunderbolt", "thunderbolt-net"]
  hardware_revisions: ["Nimo Direct MME3L / NIMO Mini PC (revision unknown)"]
related_sections: ["18", "19", "23", "49", "50"]
---

# Facts and constraints

## General architecture

- **[VERIFIED]** USB4 is a routed fabric. A connection manager enumerates routers and creates tunnels; Linux supports firmware and software connection managers [S20-01].
- **[VERIFIED]** Linux normally exposes one `domainX` per Thunderbolt/USB4 host controller and each connected router/device under `/sys/bus/thunderbolt/devices` [S20-01, S20-02]. Two domains are therefore evidence for two host-controller domains, but physical silicon/root independence still requires PCI/ACPI mapping.
- **[VERIFIED]** Host-to-host networking creates one `thunderboltN` virtual Ethernet interface per connected port through `thunderbolt-net` [S20-01]. Linux also documents USB4STREAM, but availability depends on the running kernel/config and belongs primarily to Section 50.
- **[VERIFIED]** Domain sysfs exposes security and IOMMU DMA-protection state. `user` security disables PCIe tunneling until authorization; authorization policies must not be weakened merely for benchmarks [S20-01, S20-02].
- **[VERIFIED]** USB4 link bandwidth is shared among tunneled protocols and depends on negotiated generation, lanes, cable, retimers, and platform allocation [S20-03]. Connector shape or an “USB4” label does not prove 40 Gb/s operation.

## Historical cluster observations

**[VERIFIED]** The preserved, redacted July 12 nimo-1 audit report recorded [S20-L01]:

| Rail | Netdev/address | Negotiated link | MTU | Path |
|---|---|---|---:|---|
| domain0 | thunderbolt0 / 10.44.0.1/30 | 2 × 20.0 Gb/s RX and TX | 9000 | remote nimo-2; two retimers |
| domain1 | thunderbolt1 / 10.44.0.5/30 | 2 × 20.0 Gb/s RX and TX | 9000 | remote nimo-2; two retimers |

The host routers reported `generation=4`, but the audit correctly distinguished that label from the negotiated 40 Gb/s-per-rail link. It also observed separate PCI functions `c7:00.5` and `c7:00.6`, concentrated data IRQs, no RSS/RPS/XPS, and `amd_iommu=off`.

**[MEASURED]** Preserved July 10 raw logs, timestamped environment/state output, and their checksum receipt show both rails passed traffic and MPTCP used two subflows [S20-L03]. These are functional dual-link facts, not a causal proof that simultaneous throughput doubles.

## Live topology — 2026-07-17

**[MEASURED]** Both nodes reported two authorized USB4 generation-4 host domains, and each peer path reported `rx_lanes=2`, `tx_lanes=2`, `rx_speed=20.0 Gb/s`, and `tx_speed=20.0 Gb/s` [S20-L04]. `ethtool` displayed 40,000 Mb/s for each logical interface; neither number is achieved application goodput.

| Logical rail | nimo-1 | nimo-2 | Cross-host path |
|---|---|---|---|
| A | `thunderbolt0`, `10.44.0.1`, domain0 / `c7:00.5` | `thunderbolt0`, `10.44.0.2`, domain1 / `c7:00.6` | nimo-1 domain0 ↔ nimo-2 domain1 |
| B | `thunderbolt1`, `10.44.0.5`, domain1 / `c7:00.6` | `thunderbolt1`, `10.44.0.6`, domain0 / `c7:00.5` | nimo-1 domain1 ↔ nimo-2 domain0 |

- **[MEASURED]** The MPTCP meta-socket reported two subflows, and interface counters showed cumulative multi-terabyte traffic with zero RX/TX errors and single-digit TX drops at capture [S20-L04].
- **[MEASURED]** Five ICMP echo requests in each direction on each rail had zero loss and average RTTs between 0.087 and 0.100 ms [S20-L04]. This is a reachability diagnostic, not E03 throughput or independence evidence.
- **[RECOMMENDATION]** Bind paths by subnet/address plus sysfs PCI/domain ancestry. The same `thunderbolt0` name does not map to the same domain number on both machines.

## Independence definition

Two ports are operationally independent only if:

1. each maps reproducibly to a distinct domain/NHI function and known physical port;
2. cable/port swaps exclude a single bad cable or connector;
3. simultaneous A+B load does not materially depress either rail beyond predeclared bounds compared with matched single-rail controls;
4. IRQ, CPU, memory, thermal, power, and protocol bottlenecks are measured;
5. failure of one rail leaves the other healthy and does not retrain/reset a shared controller; and
6. results repeat in both directions and on both node roles.

**[INFERENCE]** Separate domains and PCI functions make independence plausible, not proven; they may still share a root complex, fabric, memory controller, power/thermal budget, or software bottleneck.
