---
section_id: "43"
title: "Pipeline Parallel Open Questions"
status: "needs-machine-validation"
last_verified: "2026-07-16"
applies_to:
  repositories: ["HaloFPX design candidate"]
  software_versions: []
  hardware_revisions: ["dual Strix Halo premise"]
related_sections: ["32", "38", "39", "44", "45", "46", "48", "51", "52", "58"]
---

# Pipeline parallel open questions

| ID | **[OPEN]** question | Evidence needed | Owner route |
|---|---|---|---|
| DR43-O1 | Which target architectures admit a complete-block cut with a stable boundary bundle? | model graph/tensor audit and oracle | sections 29-35, `DR-43-E1` |
| DR43-O2 | Which cut minimizes the worst stage time while preserving safe memory on both ranks? | per-cut time and peak-memory matrix | `DR-43-E1`, `DR-43-E3` |
| DR43-O3 | What boundary tensors, shapes, dtypes, and metadata are required for every phase/bucket? | graph inspection and cross-rank tensor oracle | sections 32/45, `DR-43-E1` |
| DR43-O4 | Can one or two USB4 paths carry boundary bundles with acceptable p99 tails, and with which copy path? | matched link/buffer matrix | sections 51-54, `DR-43-E2` |
| DR43-O5 | What concurrency and work-unit size actually produce useful stage overlap? | offered-load/bubble sweep | sections 38/46/76, `DR-43-E3` |
| DR43-O6 | What prefill chunk size and priority bound ITL without unacceptable TTFT/throughput loss? | mixed long-prefill workload | section 46, `DR-43-E4` |
| DR43-O7 | How are sampler execution and RNG state colocated with the output head while coordinator authority remains exact? | protocol design plus deterministic/stochastic oracle | sections 39/45/48/66 |
| DR43-O8 | How are tied embeddings/heads, recurrent state, multimodal branches, native MTP, and MoE layers placed? | architecture-specific manifests | sections 33-36/44 |
| DR43-O9 | Can both rank-local KV halves be checkpointed/restored atomically, and what is the replay boundary? | cache ABI and injected-failure tests | sections 48/58-63, `DR-43-E5` |
| DR43-O10 | What bounded client behavior follows rank loss when the model cannot fit one node? | SLO/error/fallback decision and fault evidence | sections 38/48/66/68, `DR-43-E5` |
| DR43-O11 | Does ROCmFPX's current scheduler/RPC path overlap useful two-host work on gfx1151, or is a new rank-local runtime required? | traces, source audit, and prototype comparison | sections 39/45/51, `DR-43-E2/E3` |
| DR43-O12 | At what workload boundary should the planner prefer pipeline over replication or tensor parallelism? | matched correctness-gated mode matrix | sections 38/42/47/76 |

No default answer above is a verified HaloFPX fact.
