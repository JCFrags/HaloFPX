---
section_id: "76"
title: "Distributed Benchmark Open Questions"
status: "needs-machine-validation"
last_verified: "2026-07-16"
applies_to:
  repositories:
    - "ggml-org/llama.cpp@788e07dc91d266ad3162a1ce9037665656269689"
    - "charlie12345/ROCmFPX@a5605a72768c6562241b248e268e33dc92787394"
  software_versions: []
  hardware_revisions: ["planned matched dual Strix Halo"]
related_sections: ["38", "41", "42", "43", "44", "73", "75", "78", "80"]
---

# Open Questions

| ID | Question | Evidence required |
|---|---|---|
| O76-01 | Which modes are actually implemented at the frozen HaloFPX commit? | source/CLI/test inventory |
| O76-02 | What exact models/quants represent dense, MoE, hybrid, and MTP workloads? | model catalog and hashes |
| O76-03 | What output equivalence/quality tolerance admits a performance cell? | Section 78 policy |
| O76-04 | What is the matched single-node envelope on each node/backend? | Section 74 raw runs |
| O76-05 | Which message sizes, latency tails, and contention profiles constrain each mode? | Section 75 profiles |
| O76-06 | Does remote drafting overlap target work or serialize on shared resources? | event timeline and utilization |
| O76-07 | Which acceptance metric and correction semantics apply to native MTP and each draft mode? | source audit plus token traces |
| O76-08 | Which tensor collectives exist and how are they mapped across two links? | trace/source evidence |
| O76-09 | What pipeline boundaries balance compute and memory across model families? | layer timing and activation traces |
| O76-10 | Are MoE routing traces stable enough for placement, and how are cold experts handled? | repeat traces and fault cases |
| O76-11 | What epsilon, interval method, repetition count, and validity window define break-even? | Section 73 decision |
| O76-12 | What is the safe fallback for each rank/link/cache failure? | Section 48/80 evidence |
| O76-13 | How are plan results invalidated by source, model, firmware, topology, or thermal changes? | planner provenance design |

## Internet follow-up

**[OPEN]** Audit the exact frozen RPC, speculation, graph-split, collective, and MoE source paths rather than relying on moving command documentation. Identify which proposed modes are missing.

**[OPEN]** Track upstream MTP/speculative changes and unresolved issues only as scoped evidence; do not import other-backend performance reports.

## Machine follow-up

**[OPEN]** Run paired randomized matrices with raw request/rank/link events. A distributed winner cannot be selected from single-run tokens-per-second.

**[OPEN]** Validate failure detection, cancellation, rank-local cache handling, and single-node fallback before enabling automatic plan selection.
