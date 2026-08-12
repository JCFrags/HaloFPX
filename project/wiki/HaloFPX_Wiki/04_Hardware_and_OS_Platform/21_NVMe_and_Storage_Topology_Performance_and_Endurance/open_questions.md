---
section_id: "21"
title: "Storage open questions"
status: "needs-machine-validation"
last_verified: "2026-07-16"
applies_to:
  repositories: []
  software_versions: []
  hardware_revisions: ["two project Strix Halo nodes"]
related_sections: ["18", "22", "60", "65", "77", "80"]
---

# Storage open questions

| ID | Open question | Closure evidence | Owner/dependency |
|---|---|---|---|
| 21-OQ01 | **[OPEN]** What exact controllers, namespaces, firmware, capacities, and LBA formats are installed? | Paired inventory and raw identify logs | Section 18 / machine |
| 21-OQ02 | **[OPEN]** What PCIe generation and width are negotiated, and are lanes/resources shared? | `lspci -vv`, firmware/BOM, topology under both ports active | Sections 18, 20 |
| 21-OQ03 | **[OPEN]** Which filesystems, mounts, options, encryption, swap, quotas, and free-space reserves apply? | `findmnt`, `lsblk`, config authority | Section 28 |
| 21-OQ04 | **[OPEN]** What are each drive's vendor thermal limits, TBW/DWPD, warranty, and PLP behavior? | Exact part-number datasheet/warranty and controller evidence | Machine/vendor |
| 21-OQ05 | **[OPEN]** What are baseline SMART values and deltas under representative use? | Timestamped JSON before/after | Machine |
| 21-OQ06 | **[OPEN]** What model and cache footprints result from chosen artifacts and formats? | File manifests plus cache schema/rank ownership | Sections 29, 60-65 |
| 21-OQ07 | **[OPEN]** What are aligned sequential, random/metadata, mixed-load, and sustained-write results at thermal equilibrium? | Reproducible fio/application evidence | Sections 22, 73, 77 |
| 21-OQ08 | **[OPEN]** Does the commit protocol safely recover from process kill, reboot, power interruption, disk-full, and corruption? | Fault-injection assertions and raw evidence | Sections 65, 80 |
| 21-OQ09 | **[OPEN]** Which scheduler/queue/priority policy minimizes decode tail-latency impact? | Matched A/B results; reversible settings | Section 28 |

## Decision blockers

The cache device/filesystem, reserve percentage, maximum dirty tail, writeback bandwidth cap, GC policy, and default durability mode remain contingent on OQ01-OQ09.
