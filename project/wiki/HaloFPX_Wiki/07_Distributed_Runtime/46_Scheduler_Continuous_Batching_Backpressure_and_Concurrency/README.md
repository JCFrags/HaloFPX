---
section_id: "46"
title: "Scheduler, Continuous Batching, Backpressure, and Concurrency"
status: "needs-machine-validation"
last_verified: "2026-07-16"
applies_to:
  repositories: ["ggml-org/llama.cpp@788e07dc91d266ad3162a1ce9037665656269689", "vllm-project/vllm@9354f222042986addf20709e5274fc26e0d09745"]
  software_versions: []
  hardware_revisions: ["two matched AMD Strix Halo systems (exact BOM open)"]
related_sections: ["38", "39", "40", "43", "45", "48", "58", "68", "79"]
---

# Scheduler, Continuous Batching, Backpressure, and Concurrency

**[VERIFIED]** Upstream llama.cpp uses a task queue, a response queue, one slot per sequence, and a single batch shared by active slots; incompatible LoRA configurations are not co-batched [S46-LLAMA-SERVER]. **[VERIFIED]** vLLM V1 documents decode-first chunked prefill and recomputation on KV-pressure preemption [S46-VLLM-OPT]. These are useful precedents, not proof that either policy is correct for two Strix Halo ranks.

**[RECOMMENDATION]** HaloFPX should make the coordinator the sole admission and global scheduling authority. A request has one sequence owner; each participating rank executes the same immutable `step_id` schedule for sharded modes. Rank-local queues may stage work but must not independently reorder distributed collectives.

## Required invariants

1. A sequence has exactly one coordinator-owned lifecycle and monotonically increasing `step_id`.
2. A distributed batch is admitted only after every required rank reserves KV, activation, and transport credits.
3. Cancellation is an ordered command: stop scheduling new work, drain/abort the agreed step boundary, then free state on every rank.
4. No user can exceed configured queued tokens, live sequences, KV bytes, or stream-buffer bytes.
5. Overload is explicit and bounded; it never becomes unbounded queueing or silent eviction of live state.
6. Single-node fallback is a new plan/session epoch, not continuation with partially valid sharded state.

## Research split

- **Internet/source-code research completed:** pinned upstream server batching, slot, streaming, split-mode, and scheduler precedents.
- **On-machine work required:** characterize batch/ubatch limits, prefill/decode interference, dual-rank step variance, cancellation latency, cache-hit placement value, queue saturation, and per-user fairness.
- **Contingent decisions:** queue limits, prefill quantum, priority weights, overload thresholds, distributed reservation timeout, and whether preemption is recompute, checkpoint restore, or rejection.

See [facts](facts_and_constraints.md), [design](design_implications.md), [checks](procedures_and_checks.md), [open questions](open_questions.md), and [sources](sources.md).
