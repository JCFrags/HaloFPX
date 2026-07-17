---
section_id: "55"
title: "Fabric benchmark open questions"
status: "needs-machine-validation"
last_verified: "2026-07-16"
applies_to:
  repositories: ["torvalds/linux@8cdeaa50eae8dad34885515f62559ee83e7e8dda"]
  software_versions: ["Linux 7.1.3 historical baseline", "Linux 7.2-rc2 candidate"]
  hardware_revisions: ["two Nimo MME3L Strix Halo nodes; exact revisions open"]
related_sections: ["20", "50", "52", "53", "54", "73", "75", "84"]
---

# Open questions

| ID | Question | Resolution |
|---|---|---|
| S55-OQ01 | Which stable, distro-packaged kernel first carries the required USB4STREAM ABI on both nodes? | Section 23 plus S55-E01 |
| S55-OQ02 | Does the candidate kernel preserve current USB4NET, GPU, GTT, storage, and recovery behavior? | S55-E01 crossover |
| S55-OQ03 | What exact real HaloFPX message trace is complete and representative? | Sections 51/73/75 trace gate |
| S55-OQ04 | Which fixed tuning configuration enters holdout? | Separate tuning subset before E06 |
| S55-OQ05 | Does upstream USB4STREAM meet latency, real-throughput, or CPU alternative? | S55-E06 |
| S55-OQ06 | What happens under simultaneous GPU, NVMe, memory, and dual-rail load? | S55-E04 |
| S55-OQ07 | Can every rail/process failure cleanly reset epoch and restore baseline? | S55-E05 |
| S55-OQ08 | Where are CPU cycles and copies actually spent? | Approved perf/ftrace/BPF profiling |
| S55-OQ09 | Does copy-path attribution cross the registered-buffer proposal threshold? | Extension gate item 3 |
| S55-OQ10 | Can a registered-buffer lifetime/IOMMU/security ABI be upstreamable and reversible? | Separate design/review only after gate |

**[OPEN]** The accepted decision supplies thresholds, not positive results. Until experiments pass, upstream USB4NET remains the project default.
