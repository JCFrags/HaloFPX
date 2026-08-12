---
section_id: "20"
title: "USB4 Physical Topology and Dual-Port Independence"
status: "needs-machine-validation"
last_verified: "2026-07-17"
applies_to:
  repositories: []
  software_versions: ["Linux USB4/Thunderbolt", "thunderbolt-net"]
  hardware_revisions: ["Nimo Direct MME3L / NIMO Mini PC (revision unknown)"]
related_sections: ["18", "19", "23", "49", "50", "52", "55", "75", "84"]
---

# 20 — USB4 Physical Topology and Dual-Port Independence

Two working netdevs do not by themselves prove two independent physical paths. HaloFPX must establish controller/domain/root/retimer/IRQ identity and show simultaneous-load scaling with controlled cable and port swaps.

## Current disposition

- **[VERIFIED]** Preserved July 12 audit reports record `domain0/thunderbolt0` and `domain1/thunderbolt1` on each node, each negotiating two 20.0 Gb/s lanes in each direction, with two retimers in each path and `user` security [S20-L01, S20-L02]. Their raw inspection bundle was unavailable, so S20-E01 remains the topology measurement gate.
- **[MEASURED]** Preserved July 10 raw logs, environment/state output, and a checksum receipt show both netdevs carried traffic and a single MPTCP socket formed two subflows; they do not, by themselves, prove independent controllers, PCIe roots, or additive capacity under a paired experimental design [S20-L03].
- **[MEASURED]** The 2026-07-17 live capture reconfirmed two USB4 domains and two `thunderbolt-net` interfaces per node, two negotiated 20.0 Gb/s RX/TX lanes per path, MTU 9000, cumulative traffic on all four netdev endpoints, and an active MPTCP connection with two subflows [S20-L04].
- **[OPEN]** Exact physical connector mapping, retimer firmware, cable identity, IRQ sharing, simultaneous-load scaling, and statistical failure independence remain unresolved.

See [facts](facts_and_constraints.md), [implications](design_implications.md), [procedures](procedures_and_checks.md), [questions](open_questions.md), and [sources](sources.md).
