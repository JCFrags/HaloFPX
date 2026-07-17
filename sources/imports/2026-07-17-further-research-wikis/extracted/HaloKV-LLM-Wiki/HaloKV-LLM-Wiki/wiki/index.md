---
title: "HaloKV wiki index"
tags: ["index", "halokv"]
created: 2026-07-17
updated: 2026-07-17
status: design-proposal
sources: ["WIKI-01"]
related: ["Home", "References"]
---

# HaloKV wiki index

## Orientation

- [[Executive-Summary]] — design conclusions and non-negotiable invariants.
- [[Requirements-Coverage]] — traceability from requested topics to pages and artifacts.
- [[Assumptions-and-Non-Goals]] — deployment assumptions, definitions, and exclusions.
- [[System-Model]] — actors, durable objects, identity fields, and correctness properties.
- [[Protocol-Overview]] — control/data planes and end-to-end flow.
- [[Message-Schemas]] — protobuf/JSON contracts and validation semantics.
- [[Protocol-State-Machines]] — coordinator, rank, reconnect, cancellation, and degraded-mode views.

## Identity and compatibility

- [[Rank-Local-Cache-Keys]] — logical lookup keys, immutable page IDs, tenant scoping, and canonicalization.
- [[Topology-Fingerprints]] — exact-reuse and transport-compatibility fingerprints.

## Lifecycle and failures

- [[Checkpoint-Commit-Protocol]] — prepare, durable publication, global commit, abort, and garbage collection.
- [[Epochs-Retries-Cancellation]] — fencing, idempotency, ambiguous results, and cancellation linearization.
- [[Backpressure-and-Flow-Control]] — byte/page credits, bounded queues, quotas, and control-plane priority.
- [[Reconnect-and-Recovery]] — inventory exchange, delta fetch, replay, and cache-transfer avoidance.
- [[Partial-Rank-Failure]] — communicator failure, token continuity, replacement, and reconfiguration.
- [[Degraded-Mode-Behavior]] — allowed and forbidden behavior under reduced service.
- [[Fault-and-Recovery-Tables]] — machine-readable fault, recovery, degraded-mode, RPC-validation, and threat decisions.

## Integrity and security

- [[Integrity-and-Corruption]] — end-to-end digests, crash-safe publication, quarantine, scrubbing, and semantic checks.
- [[Security-Threat-Model]] — assets, trust boundaries, hostile RPC validation, authentication, authorization, and residual risk.

## Verification and operations

- [[Formal-Modeling]] — TLA+/TLC plan, model scope, invariants, liveness, Apalache, P, and Alloy roles.
- [[Fuzzing-and-Fault-Injection]] — parser, stateful, crash, network, concurrency, and corruption campaigns.
- [[Observability-and-SLOs]] — metrics, logs, traces, alerts, and illustrative service objectives.
- [[Validation-Evidence]] — schema, trace, reference-model, SANY, and TLC results for this package.
- [[Decision-Log]] — adopted architecture decisions and rejected alternatives.
- [[Open-Questions]] — implementation-specific decisions and experiments.
- [[References]] — source catalog map and research provenance.

## Machine-readable material

- `protocol/halokv.proto`
- `protocol/json-schema/*.schema.json`
- `protocol/state-machines/*.mmd`
- `protocol/examples/*.json`
- `formal/tla/*`
- `formal/p/*`
- `fuzz/*`
- `tables/*.csv`
