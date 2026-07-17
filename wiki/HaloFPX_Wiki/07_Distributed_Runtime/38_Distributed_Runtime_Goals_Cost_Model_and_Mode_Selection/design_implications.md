---
section_id: "38"
title: "Mode Selection Design Implications"
status: "needs-machine-validation"
last_verified: "2026-07-16"
applies_to:
  repositories: ["HaloFPX design candidate"]
  software_versions: []
  hardware_revisions: ["dual Strix Halo premise"]
related_sections: ["39", "40", "41", "42", "43", "44", "47", "48", "51"]
---

# Mode selection design implications

## Provisional selector

**[RECOMMENDATION]** Apply hard feasibility gates before performance ranking:

1. Reject a mode if model/tokenizer/quantization/KV compatibility hashes do not match its contract.
2. Reject a coupled mode if a required rank or fabric path is unhealthy.
3. Reject a mode if peak rank memory exceeds a measured safe limit.
4. Reject a mode without correctness and fallback tests for the exact model/runtime commits.
5. Among remaining modes, minimize measured p99 SLO violation, then energy/request or throughput according to workload class.

| Workload signal | First candidate | Reason |
|---|---|---|
| Model fits each node; concurrent independent sessions | replication | no per-token fabric dependency; doubles serving domains |
| One interactive session; compatible fast drafter; high measured acceptance | remote draft | small messages and target-only authority |
| Model does not fit one node but shardable model fits two | TP or pipeline | capacity gate dominates; compare collective vs activation tails |
| Large prefill batches and balanced stages | pipeline | microbatching can amortize bubbles |
| MoE with measured expert skew | MoE hybrid | mapping can exploit conditional activation; section 44 owns proof |

## Break-even rules

- **[RECOMMENDATION] Replication vs coupled:** choose coupled mode only if `p99(T_coupled) + switch_guard < p99(T_replica)` for the target workload, or replication fails the capacity gate. `switch_guard` must cover observed mode-switch and variance risk.
- **[RECOMMENDATION] Remote speculation:** enable only if measured accepted target tokens per verification round yields lower p99 ITL than ordinary target decode, including draft RPC, correction metadata, rollback, and cache maintenance. Disable on sustained low acceptance or draft-node tail spikes.
- **[RECOMMENDATION] TP:** require `p99(max(C0,C1)+collectives+jitter) < p99(C_single)` for both prefill and decode buckets that matter. A favorable average bandwidth is insufficient.
- **[RECOMMENDATION] Pipeline:** require microbatch throughput benefit to exceed stage imbalance and bubble/transfer cost; separately enforce interactive TTFT/ITL bounds.
- **[RECOMMENDATION] MoE hybrid:** include dispatch bytes, expert imbalance, capacity drops, and hot-replica memory in the comparison.

## Hysteresis and safety

**[RECOMMENDATION]** Select a mode at session creation and keep it stable. Permit migration only at an explicit checkpoint boundary with compatible cache identity. Require consecutive healthy measurement windows before promotion and consecutive violations before demotion; record reason, old/new mode, model identity, and cache disposition.

**[INFERENCE]** A two-node local system can use a simpler policy than a large cluster: an explicit ranked table per model/workload bucket is more auditable than an opaque online optimizer. This follows from the small topology and evidence-governance requirement, not from an upstream implementation.

## Dependency contract

- Section 40 supplies replica queue/cache telemetry.
- Section 41 supplies speculation acceptance and rollback data.
- Sections 42-44 supply exact payload formulas.
- Section 47 owns autotuning and backend selection.
- Section 48 owns correctness and degraded-mode gates.
- Section 51 supplies two-link p99 transport curves.
