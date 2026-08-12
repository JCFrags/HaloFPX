# EXP-009 — Open-Loop Saturation and Goodput

> **Wiki status:** Proposed · **Evidence state:** D0 — design only · **Last reviewed:** 2026-07-17  
> **Machine-validation status:** Not run. Missing measurements are `INSUFFICIENT_EVIDENCE`, never an implicit pass.


| Field | Value |
|---|---|
| Objective | Find the queueing knee, overload behavior, admission-control effectiveness, and maximum sustainable SLO-qualified rate. |
| Release profiles | All |
| Required evidence | M1/M2 capacity and headroom evidence |
| Estimated measured duration | 3–6 hours |
| Risk class | Medium—intentional overload |

## Decision question

Does the service degrade predictably and recover cleanly when offered load approaches and exceeds capacity?

## Hypotheses

- **H0:** Overload causes uncontrolled queues, memory growth, hangs, silent retries, or prolonged post-load degradation.
- **H1:** The knee is measurable, admission/backpressure is explicit, and the service returns to baseline after overload.

## Preconditions and provenance

- EXP-008 identifies approximate knee; request generator supports open-loop arrival independent of completion.
- Configure hard client timeouts and a test abort on unsafe memory/temperature. Production admission controls are enabled.

## Factors, controls, and run order

- Poisson offered rate at 25%, 50%, 75%, 90%, 100%, 110%, and 125% of the closed-loop knee.
- Burst sizes 4, 8, 16 at 30-second intervals.
- Topology A, B, dual; representative C2 workload mix.

## Procedure

1. Run each load point for 15 minutes after 2-minute stabilization, with randomized order below the knee and controlled ascending order above it.
2. Record submit time independent of server availability, queue/admission result, token events, errors, cancellation/retry behavior, and all telemetry.
3. After each overload point, return to 50% load for ten minutes and compare latency/goodput to pre-overload control.
4. Inspect memory/cache/connection cleanup and worker/coordinator health.
5. Identify the highest rate meeting SLOs and calculate operating headroom.

## Required measurements

- Offered rate, admitted rate, completed rate, goodput, queue depth/age, rejection/error/timeout rates, latency tails, memory growth/recovery, and post-overload delta.

All run-level data validates against [the raw-data schemas](../schemas/README.md). Use the canonical definitions in [Metric Definitions](../wiki/Metric-Definitions.md), the matrix in [`config/benchmark-matrix.yaml`](../config/benchmark-matrix.yaml), and immutable run manifests.

## Acceptance and regression rules

- No hard event, silent retry, silent output truncation, or unbounded queue/memory growth.
- At overload, requests are bounded by explicit admission, timeout, cancellation, or documented backpressure semantics.
- Within ten minutes of returning to 50% load, p95 latency and throughput return within 5% of the pre-overload control.
- Production offered rate is ≤80% of the first sustained SLO/knee limit unless a stricter approved rule exists.

A missing measurement, unmatched control, invalid cache state, unverifiable hash, or synthetic record yields `INSUFFICIENT_EVIDENCE`; it is not an implicit pass.

## Invalidation and safety-abort rules

- Open-loop generator falls behind without recording lateness.
- Client retries are untracked or counted as new independent success.
- Abort threshold is changed after seeing results.

Safety aborts remain measured events and retain all evidence. Do not exclude an unfavorable but valid run.

## Outputs

- `manifest.json`, `requests.jsonl`, `tokens.jsonl`, telemetry/log channels, and applicable correctness/fault records.
- Derived per-cell statistics with calculation code commit and paired baseline IDs.
- One experiment outcome: `PASS`, `WARN_RETEST`, `FAIL`, or `INSUFFICIENT_EVIDENCE`.

## Interpretation limits

Overload survival does not establish recovery from link or process faults; use EXP-016–018.

## Research basis

[[SRC-016]](../references/Sources.md#src-016)
