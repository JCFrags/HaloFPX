# ADR-0007: publication attempt lifecycle and operation fencing

- Status: accepted for the disabled offline L05 coordinator
- Date: 2026-07-18

## Context

ADR-0006 added an exact final-anchor compare-and-swap and forwarded a nonzero
attempt identity to that linearization point. It deliberately did not prevent a
late staging, write, publish, or synchronization callback from an abandoned or
uncertain attempt. The accepted formal model permits storage transitions only
while the attempt owner, generation, predecessor, and authority remain active.

## Decision

An injected publication backend owns a root-scoped attempt registry. Beginning
an attempt binds the nonzero 256-bit identifier to the exact expected and next
anchor identities and exact object count. Every subsequent object, manifest,
anchor, synchronization, and durable-close operation receives that identifier.
An unknown, replayed, abandoned, committed, uncertain, or differently bound
identifier is `attempt_fenced` and causes no mutation.

The lifecycle is:

```text
unused -> active_pre_anchor -> anchor_applied -> durable_closed
                            \-> uncertain
active_pre_anchor -> definitely_abandoned
active_pre_anchor -> uncertain
```

Terminal attempt identifiers are never reusable within one registry lifetime.
A definite pre-anchor failure is returned only after definite abandonment. If
abandonment cannot be confirmed, the coordinator reports
`attempt_fencing_uncertain`. A failed, interrupted, or throwing begin, anchor
replacement, anchor synchronization, or durable close is uncertain and cannot
be acknowledged. An uncertain attempt fences its root until a future separately
qualified reconciliation protocol resolves the protected anchor and registry.

Successful anchor synchronization is necessary but not sufficient for
acknowledgement. The backend must also close the exact attempt as durable. An
ordinary attempt cannot change authority epoch.

## Consequences and limits

The offline simulator uses a bounded 128-entry in-memory terminal-ID history and
one active-or-uncertain root state. This proves deterministic state-machine and
late-call rejection behavior only. The bound is a synthetic harness limit, not
a production retention policy.

No cryptographic ID generator, authenticated writer authority, persistent
attempt journal, cross-process lock, asynchronous implementation, reconciliation
procedure, authority transfer, filesystem, server hook, persistent write, node,
or durability claim is authorized here.
