---
title: Operations Runbook
description: Canary rollout, observability, failure containment, and store maintenance.
status: Proposed
evidence-date: 2026-07-17
canonical-repository: charlie12345/ROCmFPX
---

# Operations Runbook

> [!NOTE]
> Evidence is pinned to **2026-07-17**. Repository heads and source links are locked in [[Source-Register]] and `evidence/commit-lock.json`.


## Startup checks

Before enabling a persistent mode, log and verify:

- binary ROCmFPX commit and upstream base;
- compile-time feature set;
- store major/minor version;
- provider mode and write-circuit state;
- target/draft model-set digests;
- state capability mask;
- root permissions and available disk headroom;
- retention quota and staging/quarantine budgets;
- scope-key secret availability without logging the secret.

A missing strong model digest, invalid permissions, unsupported required capability, or unknown format major is a startup failure for `persistent-read-write` and a clean fallback for `off`/`ephemeral`.

## Canary progression

| Stage | Traffic | Mode | Promotion evidence |
|---|---:|---|---|
| Lab | synthetic/tiny model | read-only + disposable writer | Parser/fault suite, kill tests, deterministic restore. |
| Shadow | production-shaped, no served restore | shadow writer | Entry validation and offline replay match cold path. |
| 1% | isolated hosts/users | read-write | Zero correctness/isolation errors; bounded latency/space. |
| 10% | mixed workload | read-write | Stable p95/p99, quota and restart behavior. |
| Opt-in | explicit operators | read-write | Release acceptance and rollback drill. |

## Required metrics

- `context_store_entries{state=committed|staging|quarantine}`
- `context_store_bytes{class=payload|metadata|staging|quarantine}`
- `context_store_ops_total{op,provider,result,reason}`
- `context_store_restore_tokens` and TTFT savings distribution
- `context_store_component_bytes{component}`
- `context_store_write_circuit_open`
- `context_store_compat_reject_total{reason}`
- `context_store_scope_violation_total` (must remain zero)
- scheduler active counts by opaque scope cardinality, not raw ID
- expert telemetry overhead metrics when enabled

## Alerting

Page/rollback triggers include:

- any accepted entry with a failed post-restore deterministic check;
- any cross-scope lookup attempt;
- corruption rejection rate above baseline;
- write circuit open on more than one host;
- staging growth without commit/cleanup;
- quota overshoot beyond documented transient headroom;
- persistent hit path slower than cold path for a sustained window;
- output divergence between shadow restore and cold evaluation.

## Maintenance

- Retention deletion operates only on committed entry directories selected by rebuildable metadata.
- Staging cleanup uses age plus ownership/lock checks; it never removes an active writer directory.
- Quarantine is bounded separately and requires an operator-approved purge policy.
- Store scans run read-only and never “repair” entries in place.
- Rotating the scope-key secret creates a new namespace generation; it does not silently remap old entries.

## Multi-process rule

v1 should support either a single writer with advisory ownership or explicit multi-writer-safe directory commits. Until multi-writer acceptance is complete, a second writer must fail startup rather than share a store unsafely. Read-only scanners may run concurrently using committed entry semantics.


---

**Wiki navigation:** [[Home]] · [[Executive-Decision]] · [[Capability-Decision-Matrix]] · [[Patch-Lanes-and-Dependency-Graph]] · [[Acceptance-Criteria]]
