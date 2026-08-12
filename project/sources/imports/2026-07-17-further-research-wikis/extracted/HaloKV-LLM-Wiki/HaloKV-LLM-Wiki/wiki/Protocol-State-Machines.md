---
title: "Protocol state machines"
tags: ["state-machine", "protocol"]
created: 2026-07-17
updated: 2026-07-17
status: design-proposal
sources: ["FORMAL-01", "FORMAL-03"]
related: ["Message-Schemas", "Checkpoint-Commit-Protocol", "Formal-Modeling"]
---

# Protocol state machines

The Mermaid source files under `protocol/state-machines/` define the coordinator, rank, reconnect, cancellation, commit sequence, and degraded-mode behavior. They are design views; the TLA+ model is the primary finite-state safety model and the P sketch is the implementation-near communicating-state-machine plan.

## Shared rules

- All state-changing transitions validate authority, generation, epoch, operation identity, request digest, topology, and authorization first.
- `COMMITTED` and `ABORTED` are terminal within an operation.
- Reconnect never resumes directly from transport state; it obtains an authoritative plan.
- `FENCED` and `QUARANTINED` cannot return to active mutation without a new authorized epoch/recovery path.
- Data-plane completion is evidence for preparation only after durable object verification; it is not global commit.

## Conformance mapping

Each implementation transition should emit a structured protocol event. A trace adapter maps those events to model actions such as `Begin`, `WriteRank`, `DeliverPrepared`, `Commit`, `Cancel`, `BumpEpoch`, `Corrupt`, and `TryRead`. CI replays traces through the reference/TLA/P model and rejects any trace that violates an invariant or uses an undefined transition.
