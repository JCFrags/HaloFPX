# EXP-018 — Recovery, Rejoin, and Cycle Stability

> **Wiki status:** Proposed · **Evidence state:** D0 — design only · **Last reviewed:** 2026-07-17  
> **Machine-validation status:** Not run. Missing measurements are `INSUFFICIENT_EVIDENCE`, never an implicit pass.


| Field | Value |
|---|---|
| Objective | Prove that repeated link/process recovery leaves no stale placement, cache, socket, memory, or performance degradation. |
| Release profiles | Availability recovery; all stable dual releases |
| Required evidence | M2 repeated-cycle and post-fault equivalence |
| Estimated measured duration | 4–10 hours |
| Risk class | High—repeated disruptions |

## Decision question

After repeated failures and rejoins, is the pair equivalent to a clean start and free of cumulative leakage or degraded state?

## Hypotheses

- **H0:** Recovery accumulates resources, stale workers/caches, link state, or performance drift.
- **H1:** Repeated recovery cycles converge to the same healthy state and performance/correctness envelope.

## Preconditions and provenance

- EXP-016/017 individual scenarios pass and recovery automation/manual procedure is frozen.
- Define a canonical healthy-state fingerprint: processes, ports, workers, model placement, cache state, memory, link, and critical outputs.

## Factors, controls, and run order

- Ten alternating cycles of interface-down and worker-process restart; include three controlled worker reboot cycles when required.
- Run a fixed 100-request C2/C3 canary block after cycles 1, 5, and 10.

## Procedure

1. Capture clean-start healthy fingerprint and baseline canary/performance block.
2. Execute each fault/recovery cycle, verifying rollback and a fresh health handshake before continuing.
3. After designated cycles, compare process/socket count, RSS, caches, worker IDs, USB4 state, logs, correctness, TTFT/ITL, and throughput to clean start.
4. At cycle 10, perform a full service restart without node reboot and repeat the comparison.
5. Preserve a cycle ledger and classify any operator intervention.

## Required measurements

- Rejoin time by cycle, process/socket/memory growth, stale worker/cache count, link renegotiations, critical correctness, performance ratio to clean start, and interventions.

All run-level data validates against [the raw-data schemas](../schemas/README.md). Use the canonical definitions in [Metric Definitions](../wiki/Metric-Definitions.md), the matrix in [`config/benchmark-matrix.yaml`](../config/benchmark-matrix.yaml), and immutable run manifests.

## Acceptance and regression rules

- 10/10 software recovery cycles and required reboot cycles reach the canonical healthy state.
- No monotonic RSS/process/socket/temp-file/cache growth beyond a documented bounded cache.
- Post-cycle p95 latency and goodput remain within 5% of clean-start control; critical correctness =100%.
- No unplanned operator repair, stale worker identity, or unresolved USB4 state remains.

A missing measurement, unmatched control, invalid cache state, unverifiable hash, or synthetic record yields `INSUFFICIENT_EVIDENCE`; it is not an implicit pass.

## Invalidation and safety-abort rules

- A cycle is skipped or combined without a distinct event record.
- Clean-start control uses different runtime settings.
- Health is inferred only from an HTTP 200 without worker/placement verification.

Safety aborts remain measured events and retain all evidence. Do not exclude an unfavorable but valid run.

## Outputs

- `manifest.json`, `requests.jsonl`, `tokens.jsonl`, telemetry/log channels, and applicable correctness/fault records.
- Derived per-cell statistics with calculation code commit and paired baseline IDs.
- One experiment outcome: `PASS`, `WARN_RETEST`, `FAIL`, or `INSUFFICIENT_EVIDENCE`.

## Interpretation limits

Cycle stability is bounded by the tested fault sequence and count; it does not replace the 72-hour soak.

## Research basis

[[SRC-007]](../references/Sources.md#src-007) [[SRC-010]](../references/Sources.md#src-010)
