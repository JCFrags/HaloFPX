---
section_id: "20"
title: "USB4 topology design implications"
status: "needs-machine-validation"
last_verified: "2026-07-16"
applies_to:
  repositories: []
  software_versions: ["Linux USB4/Thunderbolt", "thunderbolt-net"]
  hardware_revisions: ["Nimo Direct MME3L / NIMO Mini PC (revision unknown)"]
related_sections: ["49", "50", "52", "55", "75", "80"]
---

# Design implications

- **[RECOMMENDATION]** Bind rail identity to domain UUID hash, PCI/NHI path, XDomain peer UUID hash, physical port label, cable label, and netdev MAC—not `thunderbolt0/1` alone.
- **[RECOMMENDATION]** Default to single-best-link plus failover until S20-E03 proves useful simultaneous scaling. Dual netdev presence is insufficient to authorize striping.
- **[RECOMMENDATION]** Keep transport policy above the link: single rail, direction split, MPTCP, application striping, and hedging must all tolerate one-rail failure and fall back without changing rank ownership.
- **[INFERENCE]** Concentrated IRQs and one queue per historical netdev can cap aggregate throughput even when physical paths are independent. CPU/IRQ tuning must be separated from physical-topology conclusions.
- **[RECOMMENDATION]** Treat `amd_iommu=off` as a security/functionality risk linked to Section 19. Do not automatically authorize arbitrary Thunderbolt devices; use peer-specific operational policy.
- **[RECOMMENDATION]** A cable pull, retrain, domain reset, or peer disappearance must produce explicit transport degradation and bounded recovery; never accept silent rerouting over the management LAN.
- **[OPEN]** Kernel USB4STREAM support and direct-stream performance are Section 50/55 decisions, not evidence that the current physical ports are independent.

## Research split

1. Completed now: USB4/Linux representation and extraction of historical topology evidence.
2. On-machine: S20-E01 topology capture, E02 physical swap matrix, E03 simultaneous-load factorial, E04 failure isolation, E05 cold-boot stability.
3. Contingent decisions: multipath policy, IRQ affinity, security/IOMMU configuration, cable replacement, and whether shared bottlenecks justify kernel/transport work.
