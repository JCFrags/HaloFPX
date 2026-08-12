---
section_id: "10"
title: "Architecture Decision Framework"
status: "needs-machine-validation"
last_verified: "2026-07-16"
applies_to:
  repositories: ["HaloFPX project"]
  software_versions: []
  hardware_revisions: ["two-node Strix Halo target"]
related_sections: ["07", "09", "38", "40", "41", "42", "43", "44", "47", "48"]
---

# Architecture decision framework

## Step 1 — Hard gates

A candidate is rejected before scoring if it lacks:

- correct output/state behavior at declared tolerances;
- model/backend/topology compatibility;
- explicit rank ownership, failure semantics, and single-node behavior;
- cache/wire integrity and version handling;
- reproducible build/test evidence and rollback;
- acceptable security/license posture.

## Step 2 — Workload-specific score

Score each surviving candidate from 1 (poor) to 5 (strong) against the same workload profile and baseline. **[RECOMMENDATION]** Use weights only after Section 07 and sponsor review; do not hide raw metrics behind the score.

| Dimension | Required evidence | Typical tension |
|---|---|---|
| Latency | p50/p95/p99 TTFT and ITL | batching/throughput; transport synchronization |
| Throughput | prompt and generated tokens/s under load | interactive tails; memory use |
| Capacity | largest passing model/context/concurrency | latency; fallback feasibility |
| Quality/correctness | task, schema, differential, cache tests | quantization/speculation speed |
| Reliability | soak, fault recovery, invalid-state rejection | optimization complexity |
| Complexity | code/patch/protocol/state-machine surface | specialized performance |
| Maintenance | upstream delta, test burden, version churn | local hardware tuning |
| Resource cost | power, thermals, RAM, SSD write/endurance | speed/capacity |

`weighted score = sum(weight_i * normalized_score_i)`, but the decision record must also show raw results, uncertainty, and sensitivity to plausible weights.

## Step 3 — Prefer the simplest sufficient mode

| Condition | Starting candidate | Escalate when |
|---|---|---|
| One model fits; interactive single user | Single-node | peer offers measured latency/capacity benefit |
| Independent concurrent requests | Replication | imbalance or model capacity blocks it |
| Predictable target with compatible draft/MTP | Local speculation, then remote speculation | remote acceptance benefit exceeds coordination cost |
| Model/context needs aggregate capacity | Tensor or pipeline parallel | only after link/correctness profiling |
| Uneven MoE expert demand | MoE-aware hybrid | representative telemetry is stable and placement wins repeatably |

## Principle consequences

- **[RECOMMENDATION]** Keep the coordinator off the per-token data path where a persistent rank protocol can safely amortize setup.
- **[RECOMMENDATION]** Cache planner results by full profile key, but invalidate on any compatibility input change.
- **[RECOMMENDATION]** Use deterministic code for collection, validation, scheduling, and compatibility; reserve model-assisted synthesis for research/review.
- **[RECOMMENDATION]** Upstream generic fixes; isolate hardware-specific tuning behind small guarded patches or data profiles.

