---
title: "Message schemas and validation contract"
tags: ["schema", "protobuf", "validation"]
created: 2026-07-17
updated: 2026-07-17
status: design-proposal
sources: ["RPC-01", "RPC-07"]
related: ["Protocol-Overview", "Security-Threat-Model", "Protocol-State-Machines"]
---

# Message schemas and validation contract

## Files

- `protocol/halokv.proto` — normative wire-shape proposal for control and data services.
- `protocol/json-schema/halokv-envelope.schema.json` — strict JSON diagnostic/control envelope.
- `protocol/json-schema/checkpoint-manifest.schema.json` — immutable global checkpoint manifest with exactly two ranks.
- `protocol/json-schema/topology-descriptor.schema.json` — canonical topology descriptor input.
- `protocol/json-schema/reconnect-plan.schema.json` — coordinator-selected reconnect action.
- `tables/rpc-validation-matrix.csv` — size, range, authorization, and semantic validation rules.

## Schema versus semantic validation

Protocol Buffers ensure field encoding, not deployment invariants. Implementations must add semantic validation for fixed digest/UUID lengths, rank authorization, generation/epoch fencing, exact topology, sorted and unique page coordinates, non-overlap, bounded counts, checked arithmetic, operation-state transitions, and manifest completeness.

The schemas deliberately avoid peer-supplied filesystem paths and arbitrary URLs. Data locations are opaque tokens minted by an authorized storage/coordinator service. Tokens are bounded, scoped, time-limited where appropriate, and never treated as object identity.

## Compatibility rules

- Major protocol version mismatch rejects the session.
- New optional fields may be ignored only when they do not enter canonical digests or alter required semantics.
- Unknown required enum values reject; default enum zero means unspecified and is invalid where the field is required.
- Canonical digest versions are explicit. Upgrading a digest canonicalization creates a new object/schema version.
- A receiver advertises maximum accepted frame, page, manifest, repeated-field, and inventory sizes during handshake; local hard caps always apply.

## Error mapping

| Condition | Suggested status |
|---|---|
| malformed, non-canonical, out-of-range | `INVALID_ARGUMENT` |
| unauthenticated transport identity | `UNAUTHENTICATED` |
| identity valid but wrong tenant/rank/action | `PERMISSION_DENIED` |
| stale epoch, wrong generation/topology, operation terminal conflict | `FAILED_PRECONDITION` or `ABORTED` |
| bounded capacity unavailable | `RESOURCE_EXHAUSTED` |
| transient peer/store unavailable | `UNAVAILABLE` |
| object/manifest integrity failure | `DATA_LOSS` |
| unsupported protocol/converter | `UNIMPLEMENTED` |

Errors expose enough detail for an authorized operator to resolve state without leaking cross-tenant object existence.
