---
section_id: "38"
title: "Mode Selection Open Questions"
status: "needs-machine-validation"
last_verified: "2026-07-16"
applies_to:
  repositories: ["HaloFPX design candidate"]
  software_versions: []
  hardware_revisions: ["dual Strix Halo premise"]
related_sections: ["40", "41", "42", "43", "44", "47", "48", "51"]
---

# Open questions

| ID | Question | Evidence needed | Owner route |
|---|---|---|---|
| DR38-O1 | What numeric p99 TTFT, ITL, throughput, and availability objectives define success? | product decision and workload trace | section 02 / decision ledger |
| DR38-O2 | What are per-link and dual-link p99 curves for real payload paths? | `DR-38-E1`/fabric experiment | section 51 |
| DR38-O3 | Are both USB4 paths independent and usable concurrently without reordering penalties? | topology, counters, fault injection | section 50-54 |
| DR38-O4 | Which target models/quantizations fit one node with required context/concurrency? | exact model hashes and peak memory | sections 27-33 |
| DR38-O5 | What is the safe mode-switch/checkpoint boundary? | cache ABI and recovery tests | sections 39, 48, 55-64 |
| DR38-O6 | Does any coupled mode beat replication at p99 under real offered load? | matched mode matrix | section 47 |
| DR38-O7 | What failure probability/cost should enter `F_m`? | long soak and injected failures | section 48 |

All entries are **[OPEN]**. No default answer should be promoted as fact.
