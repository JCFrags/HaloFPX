---
section_id: "46"
title: "Scheduler Open Questions"
status: "needs-machine-validation"
last_verified: "2026-07-16"
applies_to:
  repositories: ["HaloFPX proposed runtime"]
  software_versions: []
  hardware_revisions: ["two matched AMD Strix Halo systems (exact BOM open)"]
related_sections: ["09", "43", "48", "58", "68", "79"]
---

# Open questions

| ID | Question / gap | Closure evidence |
|---|---|---|
| S46-OQ-01 | **[OPEN]** What are the actual tenant classes, quotas, and priority semantics? | Product requirements and abuse/overload tests. |
| S46-OQ-02 | **[OPEN]** What prefill quantum and decode guard minimize p99 ITL without starving long prompts? | Mixed-workload sweeps on both backends. |
| S46-OQ-03 | **[OPEN]** Which state can be preempted and reconstructed exactly for attention, recurrent, MTP, and speculative sessions? | Model-specific state inventory plus replay tests. |
| S46-OQ-04 | **[OPEN]** How should cache-hit value trade against fairness and rank load? | Trace-driven comparison of cache-aware policies. |
| S46-OQ-05 | **[OPEN]** What reservation timeout avoids head-of-line blocking on the dual link? | Fault/latency distribution under load. |
| S46-OQ-06 | **[OPEN]** Does client disconnect cancel generation or preserve it for resumable streaming? | Product/API decision with bounded-buffer policy. |
| S46-OQ-07 | **[OPEN]** Can pipeline microbatches from different compatibility keys overlap safely? | Graph and state audit plus correctness experiment. |
| S46-OQ-08 | **[OPEN]** What full-model configurations fit one node for safe degraded operation? | Exact model/backend memory measurements. |

## Newly identified research gaps

**[OPEN]** The prompt set needs an authoritative work-cost unit shared by scheduler metrics, user accounting, and autotuning; “token” alone does not capture cached tokens, MoE experts, or distributed communication.

**[OPEN]** Streaming replay semantics need a protocol-level idempotency and visibility definition before request replay can be considered safe.
