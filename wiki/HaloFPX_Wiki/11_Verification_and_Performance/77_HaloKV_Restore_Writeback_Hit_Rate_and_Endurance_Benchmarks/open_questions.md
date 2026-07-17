---
section_id: "77"
title: "HaloKV Benchmark Open Questions"
status: "needs-machine-validation"
last_verified: "2026-07-16"
applies_to:
  repositories: ["fewtarius/CachyLLama@6be745998f568e379ea197fcf827baec73ff9940"]
  software_versions: []
  hardware_revisions: ["exact NVMe devices pending"]
related_sections: ["21", "56", "57", "58", "59", "60", "61", "62", "63", "64", "65", "73", "78", "80"]
---

# Open Questions

| ID | Question | Required evidence |
|---|---|---|
| O77-01 | What is the canonical HaloKV format, version, and commit protocol? | Sections 57/59/63 decision and tests |
| O77-02 | Which state types are restorable for dense, recurrent, hybrid, MTP, speculative, sampling, and RNG paths? | source inventory and equivalence oracle |
| O77-03 | What exact fingerprint prevents incompatible acceptance? | negative compatibility matrix |
| O77-04 | Which page/segment sizes minimize latency and write amplification by workload? | factorized sweep |
| O77-05 | How are system-prefix sharing and user isolation scoped? | security/privacy decision and adversarial traces |
| O77-06 | Does two-rank restore proceed atomically, and what happens when one rank is missing or stale? | rank protocol/fault evidence |
| O77-07 | What durability modes and acknowledged-write guarantees are exposed? | commit protocol plus crash tests |
| O77-08 | How does GC behave near quotas/full disk and during decode? | bounded pressure matrix |
| O77-09 | What are representative reuse-distance, dirty-tail, and long-context traces? | sanitized trace collection |
| O77-10 | Which exact SSD models, firmware, filesystem, TBW/DWPD, and power-loss protections exist? | node inventory and vendor datasheets |
| O77-11 | Can device-wide SMART counters be isolated with adequate resolution? | idle controls and repeated deltas |
| O77-12 | What endurance and latency budgets gate release? | product SLO/risk decision |
| O77-13 | Which power-loss tests are safe and authorized? | Section 80 experiment card |
| O77-14 | What evidence expires after model, ABI, topology, firmware, filesystem, or SSD changes? | provenance/invalidation policy |

## Internet follow-up

**[OPEN]** Pin the actual kernel, filesystem, nvme-cli, fio, drive datasheets, firmware release notes, and HaloKV source commit before executing. Current generic standards cannot establish device guarantees.

**[OPEN]** Inspect vendor telemetry only as a separately labeled extension; do not equate proprietary NAND writes with NVMe host Data Units Written.

## Machine follow-up

**[OPEN]** Begin with reversible correctness and cold/warm restore tests, then bounded pressure, and only then separately authorized fault/power tests.

**[OPEN]** Preserve failing cache images and environment metadata before repair. Corruption evidence must not be silently garbage-collected.
