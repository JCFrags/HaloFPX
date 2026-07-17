---
section_id: "07"
title: "Users, Workloads, Personas, and Use Cases"
status: "needs-machine-validation"
last_verified: "2026-07-16"
applies_to:
  repositories: ["HaloFPX project", "ggml-org/llama.cpp", "fewtarius/CachyLLama"]
  software_versions: ["llama.cpp 788e07d", "CachyLLama 6be7459"]
  hardware_revisions: ["two-node Strix Halo target"]
related_sections: ["06", "08", "09", "34", "36", "46", "60", "69"]
---

# Users and workloads

**[ASSUMPTION]** HaloFPX initially serves a trusted local operator and agent clients, with optional authenticated LAN use. No production user research or workload trace was supplied.

**[RECOMMENDATION]** Treat workloads as versioned envelopes rather than one “typical prompt.” Each envelope must specify prompt tokens, reused-prefix tokens, generated tokens, turns, concurrency, model/hash, structured-output needs, durability, privacy, and latency priority.

## Priority use cases

1. Interactive chat and coding assistance.
2. Tool-calling agents with stable system/tool prefixes and growing histories.
3. Long-lived conversations and long-context retrieval.
4. Batch evaluation, conversion, and model experimentation.
5. Authenticated multi-client service and offline operation.

See the [workload matrix](facts_and_constraints.md), [design consequences](design_implications.md), and [capture procedure](procedures_and_checks.md).

## Research split

- Internet research established available server concurrency, tool calling, timings, caching, and per-user primitives.
- Machine research must capture representative traces and measure cold/warm behavior at declared concurrency.
- Numeric workload bounds and priority ordering remain sponsor decisions.

