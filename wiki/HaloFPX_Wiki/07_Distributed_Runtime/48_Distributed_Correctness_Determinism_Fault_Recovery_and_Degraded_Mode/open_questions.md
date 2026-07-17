---
section_id: "48"
title: "Distributed Correctness Open Questions"
status: "needs-machine-validation"
last_verified: "2026-07-16"
applies_to:
  repositories: ["HaloFPX proposed runtime"]
  software_versions: []
  hardware_revisions: ["two matched AMD Strix Halo systems (exact BOM open)"]
related_sections: ["09", "53", "57", "61", "63", "66", "78", "80"]
---

# Open questions

| ID | Question | Closure evidence |
|---|---|---|
| S48-OQ-01 | **[OPEN]** What numerical/token determinism level is required for release and replay? | Product requirement plus backend/mode comparison. |
| S48-OQ-02 | **[OPEN]** Can every target model's attention, recurrent, MTP, speculative, sampler, and grammar state be serialized exactly? | State inventory and restore/replay tests. |
| S48-OQ-03 | **[OPEN]** What timeout distinguishes slow USB4/thermal behavior from failed rank without false fencing? | Soak and fault latency distributions. |
| S48-OQ-04 | **[OPEN]** Does one-link two-rank execution remain correct and useful for every collective/message class? | One-link correctness/performance matrix. |
| S48-OQ-05 | **[OPEN]** Which models/plans fit one node with required context and safety headroom? | Capacity matrix. |
| S48-OQ-06 | **[OPEN]** What exact output-commit/ack contract does the API promise? | API decision and disconnect/replay conformance suite. |
| S48-OQ-07 | **[OPEN]** Can RCCL communicator abort/rebuild meet bounded recovery on the selected ROCm build? | Version-pinned fault tests on both nodes. |
| S48-OQ-08 | **[OPEN]** Where is the durable checkpoint commit point across rank-local files? | Crash-consistency design and power-loss tests. |
| S48-OQ-09 | **[OPEN]** How are coordinator split-brain and stale worker incarnations fenced? | Lease/incarnation protocol and partition tests. |

## Newly identified gaps

**[OPEN]** Define a cross-section compatibility fingerprint used identically by plans, rank handshakes, cache objects, checkpoints, and evidence.

**[OPEN]** Define privacy handling for quarantined corrupt state and fault logs; they may contain user-derived KV or tokens.
