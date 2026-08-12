---
section_id: "28"
title: "Host System Tuning: CPU, IRQ, Scheduler, Cgroups, and Filesystems"
status: "needs-machine-validation"
last_verified: "2026-07-16"
applies_to:
  repositories: ["HaloFPX runtime"]
  software_versions: ["Linux kernel and systemd versions pending machine inventory"]
  hardware_revisions: ["matched Strix Halo hosts", "dual USB4 host links"]
related_sections: ["24", "26", "27"]
---

# Host System Tuning: CPU, IRQ, Scheduler, Cgroups, and Filesystems

## Decision summary

**[VERIFIED]** Linux exposes CPU-frequency policies, task/cgroup CPU placement, IRQ affinity, RPS/XPS, memory and CPU cgroup controls, huge-page controls, VM writeback controls, filesystem options and block queues. Their presence does not imply a beneficial setting. [S28-01, S28-02, S28-03, S28-04, S28-05, S28-06, S28-07, S28-08, S28-09]

**[RECOMMENDATION]** HaloFPX tuning is an experiment matrix, not a permanent list of sysctls. Every change needs a captured baseline, exact scope, rollback value, bounded trial, correctness check, and repeated unprofiled before/after evidence.

**[OPEN]** No governor, core partition, IRQ map, real-time policy, memory, filesystem, or NVMe tuning is approved for the actual hosts.

## Authoritative pages

- [Facts and constraints](facts_and_constraints.md)
- [Design implications](design_implications.md)
- [Reversible procedures](procedures_and_checks.md)
- [Open questions](open_questions.md)
- [Sources](sources.md)

## Research split

Internet research establishes interfaces and hazards. Both machines must supply topology, current values, supported knobs and matched trials. Deployment decisions remain contingent on section 27 traces plus correctness and tail-latency evidence.
