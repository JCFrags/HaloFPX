---
title: "Requirements coverage"
tags: ["coverage", "review", "traceability"]
created: 2026-07-17
updated: 2026-07-17
status: design-proposal
sources: []
related: ["Executive-Summary", "Fault-and-Recovery-Tables", "Validation-Evidence"]
---

# Requirements coverage

| Requested area | Normative discussion | Machine-readable or executable artifact |
|---|---|---|
| Persistent state across two execution nodes | [[System-Model]], [[Protocol-Overview]], [[Checkpoint-Commit-Protocol]] | `protocol/halokv.proto`, `protocol/json-schema/checkpoint-manifest.schema.json` |
| Rank-local cache keys | [[Rank-Local-Cache-Keys]] | manifest/page fields in `protocol/halokv.proto` |
| Topology fingerprints | [[Topology-Fingerprints]] | `protocol/json-schema/topology-descriptor.schema.json` |
| Coordinated checkpoint commits | [[Checkpoint-Commit-Protocol]] | `protocol/state-machines/commit-sequence.mmd`, `formal/tla/HaloKV.tla` |
| Epochs and stale-state rejection | [[Epochs-Retries-Cancellation]] | context/reject messages in `protocol/halokv.proto`; stale trace |
| Retries and ambiguous outcomes | [[Epochs-Retries-Cancellation]] | query/status/cancel messages; fault table |
| Cancellation | [[Epochs-Retries-Cancellation]] | `protocol/state-machines/cancellation.mmd`; cancel trace/example |
| Backpressure | [[Backpressure-and-Flow-Control]] | credit messages; RPC validation table; fuzz campaign |
| Reconnect behavior | [[Reconnect-and-Recovery]] | reconnect schema, examples, and state machine |
| Partial-rank failure | [[Partial-Rank-Failure]] | fault/degraded/recovery CSVs |
| Silent corruption | [[Integrity-and-Corruption]] | corruption trace and fuzz campaign |
| Hostile RPC inputs | [[Security-Threat-Model]], [[Message-Schemas]] | RPC validation and threat CSVs; protobuf/JSON schemas |
| Recovery without unnecessary multi-GB transfer | [[Reconnect-and-Recovery]], [[Fault-and-Recovery-Tables]] | recovery-options CSV and reconnect plan examples |
| Formal modeling | [[Formal-Modeling]] | TLA+/TLC model/config/results and P sketch |
| Fuzzing and fault injection | [[Fuzzing-and-Fault-Injection]] | campaign YAML, dictionary, reference model/tests/traces |
| Degraded-mode behavior | [[Degraded-Mode-Behavior]] | degraded-mode CSV and Mermaid state machine |
| Explicit single-node feasibility | [[Partial-Rank-Failure]], [[Degraded-Mode-Behavior]], [[Fault-and-Recovery-Tables]] | per-row verdicts in three CSVs; reconnect boolean/reason fields |
| Threat model | [[Security-Threat-Model]] | threat-matrix CSV |
| Validation evidence | [[Validation-Evidence]] | `validation/`, `scripts/lint-wiki.py`, `scripts/deep-validate.py` |

No recovery mechanism is intentionally left with an implicit single-node outcome. The linter rejects blank or unclassified verdicts in `fault-matrix.csv`, `recovery-options.csv`, and `degraded-modes.csv`.
