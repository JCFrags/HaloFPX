---
section_id: "07"
title: "Workload Design Implications"
status: "needs-machine-validation"
last_verified: "2026-07-16"
applies_to:
  repositories: ["HaloFPX project"]
  software_versions: []
  hardware_revisions: ["two-node Strix Halo target"]
related_sections: ["08", "09", "10", "38", "40", "41", "46", "60"]
---

# Design implications

| Workload signal | Implication |
|---|---|
| Stable agent prefix | **[RECOMMENDATION]** Measure exact-prefix reuse and persistent restore; hash all compatibility inputs and miss safely. |
| Interactive streaming | **[RECOMMENDATION]** Optimize p50/p95 TTFT and inter-token latency; throughput alone is insufficient. |
| Many independent jobs | **[RECOMMENDATION]** Prefer replication/continuous batching when it beats communication-heavy splitting. |
| Model exceeds one-node practical envelope | **[INFERENCE]** Distribution may be required for capacity even when it does not improve latency. |
| Tool calls/JSON | **[RECOMMENDATION]** Make schema validity and task correctness quality gates for quantization and speculation. |
| Long-lived conversation | **[RECOMMENDATION]** Treat session identity, cache lifecycle, model/template compatibility, and backup as product data concerns. |
| Multiple users | **[RECOMMENDATION]** Enforce authentication, per-user state isolation, quotas/backpressure, and observable fairness. |
| Offline use | **[RECOMMENDATION]** Avoid mandatory cloud control paths; make updates and model imports explicit offline operations. |
| Mixed models | **[RECOMMENDATION]** Record load/swap cost and avoid promising simultaneous residency until measured. |

## Mode hypotheses to test

- **[INFERENCE]** Replication is the simplest candidate for independent concurrent requests.
- **[INFERENCE]** Remote speculation may help predictable generation only when acceptance gains exceed link and coordination cost.
- **[INFERENCE]** Tensor or pipeline parallelism is primarily relevant when capacity requires both nodes or matched measurements show latency/throughput benefit.
- **[INFERENCE]** MoE-aware placement requires representative expert traces; synthetic uniform routing is not sufficient.

## Product defaults

**[RECOMMENDATION]** Default selection must be conservative: single-node or replication until a distributed mode has a compatible, reproducible profile. Never select a mode merely because two nodes are available.

