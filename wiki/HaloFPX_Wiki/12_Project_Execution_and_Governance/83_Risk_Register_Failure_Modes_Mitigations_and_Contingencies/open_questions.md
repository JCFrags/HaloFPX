---
section_id: "83"
title: "Risk Register Open Questions and Cross-Project Research Gaps"
status: "needs-machine-validation"
last_verified: "2026-07-17"
applies_to:
  repositories: ["HaloFPX integration repository"]
  software_versions: []
  hardware_revisions: ["dual Strix Halo; exact BOM open"]
related_sections: ["11", "15", "18", "20", "21", "22", "23", "29", "31", "48", "55", "63", "71", "78", "79", "80", "82", "84", "85", "86"]
---

# Open questions and cross-project research gaps

All items are **[OPEN]**. Risk scores do not answer them.

## Identifier crosswalk

Section 03 defines `HLX-OQ-NNNN` as the canonical open-question form. `OQ83-*` values below are immutable section-local aliases retained for existing references. The following collision-free targets are **[RECOMMENDATION]** candidate allocations, not approved canonical records; before any question is cited outside Section 83, the naming authority must approve and register the target, preserve the alias, and pass collision/dangling-reference checks. Until then, external consumers must link this page and must not represent a candidate as canonical.

| Local alias | Candidate canonical target | Allocation state |
|---|---|---|
| `OQ83-01` | `HLX-OQ-8301` | **[OPEN]** pending naming-authority approval |
| `OQ83-02` | `HLX-OQ-8302` | **[OPEN]** pending naming-authority approval |
| `OQ83-03` | `HLX-OQ-8303` | **[OPEN]** pending naming-authority approval |
| `OQ83-04` | `HLX-OQ-8304` | **[OPEN]** pending naming-authority approval |
| `OQ83-05` | `HLX-OQ-8305` | **[OPEN]** pending naming-authority approval |
| `OQ83-06` | `HLX-OQ-8306` | **[OPEN]** pending naming-authority approval |
| `OQ83-07` | `HLX-OQ-8307` | **[OPEN]** pending naming-authority approval |
| `OQ83-08` | `HLX-OQ-8308` | **[OPEN]** pending naming-authority approval |
| `OQ83-09` | `HLX-OQ-8309` | **[OPEN]** pending naming-authority approval |
| `OQ83-10` | `HLX-OQ-8310` | **[OPEN]** pending naming-authority approval |
| `OQ83-11` | `HLX-OQ-8311` | **[OPEN]** pending naming-authority approval |
| `OQ83-12` | `HLX-OQ-8312` | **[OPEN]** pending naming-authority approval |
| `OQ83-13` | `HLX-OQ-8313` | **[OPEN]** pending naming-authority approval |
| `OQ83-14` | `HLX-OQ-8314` | **[OPEN]** pending naming-authority approval |
| `OQ83-15` | `HLX-OQ-8315` | **[OPEN]** pending naming-authority approval |

| ID | Cross-project question | Evidence needed | Owning sections | Risks |
|---|---|---|---|---|
| OQ83-01 | Are the two physical ports independent in controller/NHI, PCIe/ACPI ancestry, IRQ, retimer, clock, power, and thermal resources? | Paired topology inventory plus isolated/simultaneous saturation | 18, 20, 75 | 001, 002 |
| OQ83-02 | What sustained useful bandwidth/latency/loss is available per rail and together under actual inference traffic? | Preregistered fabric and end-to-end matrix with confidence intervals | 49-55, 75, 76 | 001, 002, 004 |
| OQ83-03 | Which exact lane-aware OS/kernel/firmware/ROCm/HIP/HSA/RCCL/Mesa tuple is installed, supported, correct, reproducible, and rollback-safe on both nodes? | Two-node component inventory, lane declaration, ancestry, smoke, soak, and rollback receipts | 18, 23, 70, 81, 85 | 003, 010 |
| OQ83-04 | Which HIP/Vulkan backend wins for each required model/op/shape after correctness and memory controls? | Matched shape-complete benchmark and correctness matrix | 24, 25, 37, 74, 78 | 004, 006, 007 |
| OQ83-05 | What is the minimal provenance-complete patch stack, and what recurring sync effort is supportable? | Commit-to-symbol map, license review, two sync rehearsals, measured effort | 11, 13-16, 85 | 005, 014, 017 |
| OQ83-06 | What models, quantizations, contexts, samplers, MTP/recurrent states, and quality thresholds are actually supported? | Immutable model catalog, conversion provenance, golden/quality/state suite | 29-36, 57, 61, 78 | 006 |
| OQ83-07 | What admission envelope preserves recovery headroom across unified memory, workspaces, transport, cache, and concurrency? | Allocation accounting, pressure sweep, fragmentation/recovery tests | 19, 32, 46, 68, 79 | 007 |
| OQ83-08 | What exact NVMe devices/firmware/TBW/filesystems exist, and what application/device write amplification and life policy apply? | BOM/SMART/datasheet plus representative write-accounting experiment | 18, 21, 62, 65, 77 | 008, 015 |
| OQ83-09 | What sustained thermal/power envelope is safe at target ambient with compute, fabric, and storage active together? | Calibrated multi-domain soak, throttling/error data, product limit decision | 22, 79 | 009 |
| OQ83-10 | Who are the security principals and accepted exposure boundary, and what peer/client identity, encryption, sandbox, and key lifecycle are required? | Threat model, ADRs, negative tests, recovery/rotation drill | 53, 64, 71, 72 | 013, 015, 017 |
| OQ83-11 | Which named people can independently build, recover, review, and operate each critical surface, and what work can be deferred if capacity is absent? | Ownership matrix, availability, independent runbook drill | 82, 86 | 011, 012 |
| OQ83-12 | What schedule range follows from unresolved research, integration effort, review capacity, hardware access, and rework reserve? | Dependency-aware estimate updated from completed spikes | 82, 84, 86 | 011, 012 |
| OQ83-13 | What checksum, compatibility fingerprint, atomic commit, distributed durability, quarantine, migration, and secure deletion contracts govern HaloKV? | ADR plus corruption/crash/power-loss/privacy tests | 57-65, 72, 77, 80 | 008, 013, 015, 016 |
| OQ83-14 | What fencing, timeout, output-commit, replay, and single-node fallback semantics hold for every distributed mode? | Protocol decision plus cable/rank/coordinator fault matrix | 38-48, 68, 80 | 016, 018 |
| OQ83-15 | What upstream-watch cadence, emergency-patch SLA, baseline support window, risk-acceptance authority, and release evidence bundle are sustainable? | Two update rehearsals, governance ADR, offline restoration and audit | 15, 16, 72, 81, 85, 86 | 005, 011, 012, 014, 017 |

## Highest-leverage dependency chain

**[INFERENCE]** OQ83-01 through OQ83-04 decide whether bandwidth-coupled dual-node execution is worth implementing. OQ83-06/07 decide whether target workloads fit and remain correct. OQ83-08/09/13 decide whether persistent caching is sustainable and safe. OQ83-10/14 decide whether the product can be exposed beyond a single trusted process. OQ83-11/12/15 decide whether any technically viable design is maintainable.

## Closure rule

Close an item only with a stable evidence or decision link, exact applicability, reviewer, and affected risk-score update. If the answer varies by model, backend, hardware, or software tuple, replace the broad question with versioned matrix cells rather than a universal conclusion.
