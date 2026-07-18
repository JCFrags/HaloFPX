---
section_id: "55"
title: "Fabric benchmark sources"
status: "needs-machine-validation"
last_verified: "2026-07-17"
applies_to:
  repositories: ["torvalds/linux@8cdeaa50eae8dad34885515f62559ee83e7e8dda"]
  software_versions: ["Linux 7.1.3 historical baseline", "Linux 7.2-rc2 candidate"]
  hardware_revisions: ["two Nimo MME3L Strix Halo nodes; exact revisions open"]
related_sections: ["20", "50", "53", "54", "73", "75"]
---

# Sources

Web sources accessed 2026-07-16.

| ID | Primary source / revision | Supports | Limitations |
|---|---|---|---|
| S55-01 | Linux v7.2-rc2 commit `8cdeaa50eae8dad34885515f62559ee83e7e8dda`, [`drivers/thunderbolt/stream.c`](https://github.com/torvalds/linux/blob/v7.2-rc2/drivers/thunderbolt/stream.c) | exact file operations, copies, frame/ring behavior | Release candidate; not project runtime |
| S55-02 | Linux v7.2-rc2, [`configfs-thunderbolt_stream`](https://github.com/torvalds/linux/blob/v7.2-rc2/Documentation/ABI/testing/configfs-thunderbolt_stream) and [administrator guide](https://github.com/torvalds/linux/blob/v7.2-rc2/Documentation/admin-guide/thunderbolt.rst) | testing ABI, provisioning, coexistence | ABI may evolve before stable |
| S55-03 | Linux perf tools, [`perf-stat(1)` manual](https://man7.org/linux/man-pages/man1/perf-stat.1.html), rendered 2026-04-18; pair with `perf stat --help` from the exact installed tool | system-wide counter collection and machine-readable output | Exact events/support are machine-specific |
| S55-04 | IETF RFC 5481, [Packet Delay Variation Applicability Statement](https://www.rfc-editor.org/rfc/rfc5481), March 2009 | latency variation terminology | Network-level guidance; project statistics are preregistered separately |

## Scoped project evidence

- **S55-L01:** `C:/Users/britt/AgentRoot/10_projects/11_active/11.02_strix-halo-usb4-cluster/01_discovery/output/2026-07-12__m2-usb4stream-transport__feasibility-plan__v01.md`, reviewed subject hash recorded by D-035. Supports historical state, source audit, matrix, statistics and rollback plan. It is a plan with historical anchors, not execution.
- **S55-L02:** `C:/Users/britt/AgentRoot/10_projects/11_active/11.02_strix-halo-usb4-cluster/DECISIONS.md`, accepted D-2026-07-12-035, 2026-07-12. Governs default/fallback and accepted thresholds; authorizes no node change or performance claim.
- **S55-L03:** `C:/Users/britt/AgentRoot/10_projects/11_active/11.02_strix-halo-usb4-cluster/03_validation/output/2026-07-12__m0-usb4net-transport-baseline__report__v01.md`, canonical replacement-run report dated 2026-07-12; accessed 2026-07-17. Directly supports the historical Linux `7.1.3-1-cachyos`, dual-rail USB4NET, aggregate/per-rail throughput, latency, and environment claims. It identifies replacement run `2026-07-12T015409-0700__m0-usb4net-transport-baseline-rerun-v02`, retained 1,416-file raw tree, normalized evidence, strict schemas, integrity manifest, source bundles, raw-tree SHA-256 `221544f84015ea744cba300c81bf9e742d9746079bd9d62b2eed4ffed510475d`, and measurement-core SHA-256 `ea06080c383c39e1a0c2992d84785366cd31dfdfb718ae8c87bb27adf7721ae3`. Limitations: no model or USB4STREAM was loaded; three throughput repetitions do not support p99; retained latency quantiles cannot be independently recomputed from discarded individual observations; historical evidence is not current admission.
- **S55-L04:** `experiments/2026-07-17-usb4-transport-baseline/`, current paired-node control with exact scope, commands, raw iperf JSON/server logs, loaded ping summaries, MPTCP socket evidence, results, and safety observations. Supports current one-sample per-rail/concurrent/MPTCP goodput, construction, latency, and cleanup claims. Limitations: one throughput sample per cell; no full CPU/IRQ/perf capture, tensor workload, bidirectional simultaneous traffic, fault campaign, or USB4STREAM.
