---
section_id: "48"
title: "Distributed Correctness Design Implications"
status: "needs-machine-validation"
last_verified: "2026-07-16"
applies_to:
  repositories: ["HaloFPX proposed runtime"]
  software_versions: []
  hardware_revisions: ["two matched AMD Strix Halo systems (exact BOM open)"]
related_sections: ["39", "42", "43", "45", "46", "53", "57", "58", "63"]
---

# Design implications

## Ownership and ordering

**[RECOMMENDATION]** The coordinator owns request identity, visible output ledger, sampling/RNG state, session epoch, and the canonical ordered command log. Rank ownership is frozen by the plan: tensor ranks own tensor/KV shards; pipeline ranks own contiguous layer KV; remote draft state is disposable while the target owns canonical state; replication gives one node complete sequence ownership.

Every command carries `(session_id, epoch, step_id, command_seq, plan_hash, state_prefix_hash)`. Workers accept only the next expected command and exact compatibility fingerprint. Collective type/count/dtype/shape/order is derived from the committed step descriptor; divergence aborts the epoch.

## Failure matrix

| Fault | Required behavior | Safe fallback |
|---|---|---|
| One link lost | Stop assigning it; complete only operations whose integrity/completion is proven. | Continue on one link only under a prevalidated plan; otherwise abort epoch. |
| Worker/rank timeout or restart | Fence old worker generation; abort communicator and uncommitted step; handshake new incarnation ID. | Restore mutually validated checkpoint or restart prompt under new plan. |
| Model/tokenizer/build mismatch | Reject worker before state allocation. | Use matching node or single-node plan. |
| Cache fingerprint/hash mismatch or corruption | Treat as miss; quarantine invalid object; never consume it. | Recompute from tokens/valid checkpoint. |
| Cancellation | Ordered, idempotent cancel at declared boundary; release all rank state after acknowledgement/fencing. | Terminal cancelled status; no silent replay. |
| Output disconnect | Do not confuse transport loss with generation cancellation. | Bounded resumable ledger or explicit cancellation policy. |

## Replay and visibility

**[RECOMMENDATION]** Assign each emitted token a monotonically increasing output index and append it to a bounded/durable visibility ledger before or atomically with external delivery. A replay request includes last acknowledged index. Never resample already committed tokens. If exact sampler/state restoration is unavailable, return terminal failure or regenerate as a new response identity, not as transparent continuation.

## Degraded mode

**[RECOMMENDATION]** Precompute manifests for dual-link/two-rank, one-link/two-rank if validated, and single-node. Transition only at a checkpoint/step boundary and always increment epoch. Full replication can redirect new requests immediately; an in-flight request needs validated full state or prompt replay. Tensor/pipeline/MoE sharded state cannot be reinterpreted as single-node state.
