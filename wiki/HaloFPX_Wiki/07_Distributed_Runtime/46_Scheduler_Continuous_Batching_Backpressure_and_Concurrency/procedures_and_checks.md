---
section_id: "46"
title: "Scheduler Procedures and Checks"
status: "needs-machine-validation"
last_verified: "2026-07-16"
applies_to:
  repositories: ["HaloFPX proposed runtime"]
  software_versions: []
  hardware_revisions: ["two matched AMD Strix Halo systems (exact BOM open)"]
related_sections: ["45", "48", "73", "79", "80"]
---

# Procedures and checks

## Internet/source-code follow-up

1. Diff pinned llama.cpp server scheduling and cancellation code against the frozen integration baseline; record paths and commits.
2. Trace ROCmFPX batch/ubatch, graph-capture, RPC, and backend constraints at its frozen commit; do not assume upstream behavior survived the fork.
3. Track vLLM scheduler work only as an algorithm reference; cite the exact source revision for any adopted policy.

## On-machine experiment matrix

Prerequisites: both machines on pinned firmware/software; synchronized clocks; fixed model hash; raw metrics and configuration retained. Root is not required unless applying host tuning.

1. Run one-node HIP and Vulkan baselines for prompt lengths `{128, 2k, 16k}`, output lengths `{32, 256}`, and concurrency `{1,2,4,8}`.
2. Repeat for replication, draft, tensor, and pipeline plans that are actually supported. Record rank timelines and plan ID.
3. Sweep batch tokens, ubatch, slot count, and prefill quantum. Randomize run order; repeat enough to report distributions and confidence intervals.
4. Mix short interactive decode with long prefills. Assert a configured maximum starvation interval and measure TTFT/ITL by user.
5. Saturate each bound separately: queue tokens, KV, rank ring, transport credits, and stream buffer. Verify bounded rejection and recovery.
6. Cancel while queued, during prefill, during collective execution, and after partial streaming. Assert no leaked KV/lease and no later tokens.
7. Kill or disconnect one rank during admission and execution; verify behavior against section 48, including safe single-node restart only under a new epoch.

## Pass criteria to set before running

**[RECOMMENDATION]** Pre-register SLOs and overload expectations. At minimum: no invariant violation; no unbounded structure; no cross-user starvation; all admitted distributed work has all-rank reservations; cancellations converge; repeated overload returns to baseline resource use. Numerical thresholds are **[OPEN]** until section 09/73 requirements are authoritative.
