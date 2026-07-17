---
section_id: "50"
title: "USB4STREAM and thunderbolt-net - Sources"
status: "draft"
last_verified: "2026-07-17"
applies_to:
  repositories: ["linux@fce2dfa773ced15f27dd27cd0b482a7473cdcf2a"]
  software_versions: ["Linux 7.2-rc3-era master"]
  hardware_revisions: []
related_sections: ["20", "49", "52", "53", "55"]
---

# Sources

## S50-L01 — Live carrier capability inventory

- Canonical source: [`../../../../sources/measurements/2026-07-17-strix-halo-live-inventory/README.md`](../../../../sources/measurements/2026-07-17-strix-halo-live-inventory/README.md)
- Capture: both nodes, 2026-07-17.
- Supports: current kernel/config, active dual USB4NET interfaces, MPTCP two-subflow use, and absence of a USB4STREAM module/device.
- Limitations: does not test a 7.2 kernel or backport and does not compare carrier performance.

| ID | Primary source and revision | Claims supported | Limitations |
|---|---|---|---|
| S50-01 | [Linux `drivers/thunderbolt/stream.c`](https://github.com/torvalds/linux/blob/fce2dfa773ced15f27dd27cd0b482a7473cdcf2a/drivers/thunderbolt/stream.c), commit `fce2dfa773ced15f27dd27cd0b482a7473cdcf2a`, 2026-07-16; accessed 2026-07-16 | packet sizes/types, HopIDs, rings, throttling, file operations, errors | current master snapshot; no target result |
| S50-02 | [Linux USB4 and Thunderbolt guide at pinned commit](https://github.com/torvalds/linux/blob/fce2dfa773ced15f27dd27cd0b482a7473cdcf2a/Documentation/admin-guide/thunderbolt.rst), 2026-07-16; accessed 2026-07-16 | setup, discovery, coexistence, net interface and security guidance | examples require observed XDomain substitution |
| S50-03 | [Linux 6.16 versioned Thunderbolt guide](https://docs.kernel.org/6.16/admin-guide/thunderbolt.html), 2025 release line; accessed 2026-07-16 | absence boundary: networking documented, stream section absent | absence is not hardware incompatibility proof |
| S50-04 | [Linux `drivers/net/thunderbolt.c` at pinned commit](https://github.com/torvalds/linux/blob/fce2dfa773ced15f27dd27cd0b482a7473cdcf2a/drivers/net/thunderbolt.c), 2026-07-16; accessed 2026-07-16 | implementation authority for ThunderboltIP networking | not benchmarked here |
| S50-05 | [Linux MPTCP documentation](https://docs.kernel.org/networking/mptcp.html), rolling docs accessed 2026-07-16 | possible TCP multipath control surface | MPTCP is separate from direct USB4STREAM |
| S50-06 | [Linux commit `6db21d817b43f8ce5654ccc7aff80d40e4dba4ac`](https://github.com/torvalds/linux/commit/6db21d817b43f8ce5654ccc7aff80d40e4dba4ac), "thunderbolt: Add support for USB4STREAM", 2026-05-19; accessed 2026-07-16 | introduction commit and mainline provenance | does not establish distro or hardware support |
| S50-07 | Local Agent Harness `guide/architecture.md`, read 2026-07-16 | evidence and promotion rules | governance only |

No third-party throughput number was promoted. Kernel compatibility and performance remain machine experiments.
