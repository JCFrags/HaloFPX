---
section_id: "55"
title: "Fabric Microbenchmark Plan and USB4 Kernel-Patch Decision"
status: "needs-machine-validation"
last_verified: "2026-07-17"
applies_to:
  repositories: ["torvalds/linux@8cdeaa50eae8dad34885515f62559ee83e7e8dda"]
  software_versions: ["current project kernel 7.1.3-1-cachyos (historical)", "Linux 7.2-rc2 USB4STREAM candidate"]
  hardware_revisions: ["two Nimo MME3L Strix Halo nodes; exact revisions open"]
related_sections: ["20", "49", "50", "52", "53", "54", "73", "75", "76", "84"]
---

# 55 — Fabric Microbenchmark Plan and USB4 Kernel-Patch Decision

This is the decision gate, not a benchmark result. It compares identical framed workloads across same-kernel TCP/USB4NET and upstream USB4STREAM, then permits kernel-extension research only if correctness, rollback, real-workload value, and copy-path attribution all pass.

## Governing baseline

- **[MEASURED]** The retained canonical 2026-07-12 no-model transport report recorded both USB4NET rails and aggregate means of about 20.8 Gb/s in several four-stream cells on Linux `7.1.3-1-cachyos` [S55-L03]. The report links the 1,416-file raw tree, normalized samples, schemas, manifests, source bundles, and raw-tree digest. This is historical configuration-scoped context, not a current control or inference result.
- **[VERIFIED]** Linux 7.2-rc2 USB4STREAM is a new testing ABI/driver with page-copy semantics and is absent from the historically running 7.1.3 kernels [S55-01, S55-L01].
- **[RECOMMENDATION]** The accepted D-2026-07-12-035 decision remains authoritative: USB4NET/TCP/MPTCP stays default; USB4STREAM is an optional reversible probe and must earn advancement [S55-L02].
- **[OPEN]** No kernel patch, performance win, GPU-to-peer-GPU result, or production security claim exists.

## Decision outcomes

1. Retain upstream TCP/USB4NET if any prerequisite, correctness, recovery, or benefit gate fails.
2. Use upstream USB4STREAM only as an optional backend if it passes the accepted one-million-message and paired performance/CPU gate.
3. Consider a registered-buffer kernel extension only after upstream correctness passes, the real workload still misses the benefit gate, and profiling plus a safe prototype proves the current page copies are the material removable bottleneck.

See [facts](facts_and_constraints.md), [implications](design_implications.md), [procedures](procedures_and_checks.md), [questions](open_questions.md), and [sources](sources.md).
