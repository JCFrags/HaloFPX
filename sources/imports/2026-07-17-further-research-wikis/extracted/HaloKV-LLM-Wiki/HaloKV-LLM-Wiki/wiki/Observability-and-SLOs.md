---
title: "Observability and SLOs"
tags: ["observability", "metrics", "slo"]
created: 2026-07-17
updated: 2026-07-17
status: design-proposal
sources: ["RPC-03", "RPC-06"]
related: ["Degraded-Mode-Behavior", "Fuzzing-and-Fault-Injection"]
---

# Observability and SLOs

## Metrics

Recommended metric families:

```text
halokv_checkpoint_operations_total{outcome,reason,durability}
halokv_checkpoint_commit_latency_seconds{durability}
halokv_checkpoint_prepare_latency_seconds{rank}
halokv_checkpoint_bytes{kind=logical|new|deduplicated|uploaded}
halokv_recovery_bytes{kind=local|fetched|recomputed|avoided}
halokv_recovery_tokens_replayed_total
halokv_epoch_current{session_hash}
halokv_epoch_bumps_total{reason}
halokv_stale_messages_rejected_total{kind,rank}
halokv_topology_mismatch_total{source,target}
halokv_integrity_failures_total{layer,source}
halokv_quarantined_objects
halokv_transfer_inflight_bytes{tenant_class,peer}
halokv_transfer_credit_bytes{peer}
halokv_checkpoint_backlog{tenant_class}
halokv_orphan_bytes{epoch}
halokv_degraded_mode{mode}
```

Avoid raw tenant/session labels with unbounded cardinality; use sampled traces, hashed identifiers, or admin-only lookup.

## Structured events

Record `CheckpointBegan`, `RankPrepared`, `CommitLinearized`, `CheckpointAborted`, `EpochGranted`, `PeerFenced`, `ReconnectPlanned`, `PageQuarantined`, `RecoveryAttached`, and `SingleNodeDecision`. Each event includes operation identity, authority revision, topology fingerprint prefix, logical position, durability class, sizes, elapsed time, and reason code—not prompts, token text, page bytes, credentials, or raw storage URLs.

## Tracing

One trace should connect client generation, decode boundary, coordinator operation, rank snapshots, object-store calls, authority CAS, and reconnect recovery. Preserve `op_id` as baggage only inside trusted services. Mark the commit CAS span as the linearization point and distinguish timeout from authoritative outcome.

## Alerts

High-value alerts include stale-epoch acceptance attempts, conflicting `op_id` digests, any digest collision/object mismatch, repeated authority revision rollback, integrity failures above baseline, orphan growth without GC progress, checkpoint lag beyond policy, control-plane credit starvation, duplicate active instances for a rank, and repeated single-node reconfiguration failure.

## Illustrative objectives

These are placeholders for benchmarking, not sourced production targets:

- 99.9% of strict checkpoints commit within a configured fraction of the checkpoint interval under nominal load.
- No committed checkpoint is materialized with fewer than all expected ranks.
- 100% of stale epoch mutation attempts are rejected in conformance tests.
- Recovery fetch bytes are no greater than the verified missing/corrupt object set plus bounded protocol overhead.
- Cancellation/status control messages remain serviceable during maximum allowed data-plane load.
- Crash-fault campaigns produce only the prior certificate, the new certificate, or an aborted/uncommitted operation—never a mixed visible checkpoint.

## Audit questions

Operators should be able to answer: Which authority revision committed this checkpoint? Which exact ranks and topology produced it? Where is each object durable? How many bytes were reused versus fetched? Which visible tokens were replayed? Why was single-node continuation accepted or rejected?
