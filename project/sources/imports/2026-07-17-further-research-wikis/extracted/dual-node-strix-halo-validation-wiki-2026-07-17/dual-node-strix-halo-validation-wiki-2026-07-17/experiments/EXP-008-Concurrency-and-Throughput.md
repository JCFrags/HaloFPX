# EXP-008 — Concurrency and Throughput Matrix

> **Wiki status:** Proposed · **Evidence state:** D0 — design only · **Last reviewed:** 2026-07-17  
> **Machine-validation status:** Not run. Missing measurements are `INSUFFICIENT_EVIDENCE`, never an implicit pass.


| Field | Value |
|---|---|
| Objective | Measure request throughput, aggregate token rate, goodput, queueing, and latency as concurrency increases. |
| Release profiles | All |
| Required evidence | M1/M2 serving envelope |
| Estimated measured duration | 15 minutes or 1,000 requests per load point |
| Risk class | Low |

## Decision question

At what concurrency does each topology maximize SLO-qualified output without unacceptable latency, errors, or resource pressure?

## Hypotheses

- **H0:** Apparent throughput comes from SLO violations, failed requests, cache imbalance, or unstable queues.
- **H1:** A repeatable operating point and headroom margin are established using goodput rather than raw offered load.

## Preconditions and provenance

- EXP-007 client instrumentation passes; cache policy, slots, scheduler/batching, admission control, and client request mix are frozen.
- Use matched prompt IDs and output limits across topologies. Report cache-hit and no-hit mixes separately.

## Factors, controls, and run order

- Closed-loop concurrency 1, 2, 4, 8, 16, and additional points around the observed knee.
- Workload mix: short interactive, medium prefill, long context, with declared weights.
- Topology A, B, dual; at least 2-minute warm-up and 15-minute/1,000-request measured window.

## Procedure

1. Run a low-load canary and verify state/correctness.
2. Execute load points in randomized up/down order to detect thermal or cache hysteresis.
3. Record offered/completed request rate, prompt/output tokens, all latency events, queue depth, active/waiting requests, cache counters, and resource/link telemetry.
4. Calculate raw throughput and SLO-qualified goodput. Keep timeouts, cancellations, and failures in the submitted denominator.
5. Repeat the selected knee and production points in an independent block.

## Required measurements

- Requests/s, aggregate prompt/output tokens/s, `GOODPUT-OTPS`, TTFT/ITL/E2E distributions, queue growth, success, cache rates, power/energy, and resource/link utilization.

All run-level data validates against [the raw-data schemas](../schemas/README.md). Use the canonical definitions in [Metric Definitions](../wiki/Metric-Definitions.md), the matrix in [`config/benchmark-matrix.yaml`](../config/benchmark-matrix.yaml), and immutable run manifests.

## Acceptance and regression rules

- Production point meets every absolute latency/correctness/success SLO with at least 20% offered-load headroom to the first failed/knee point.
- Candidate goodput is not >5% below eligible baseline.
- No monotonically growing queue for three consecutive windows, memory spiral, swap, hard event, or unbounded cancellation cleanup.
- Scale-out stable target: dual SLO-qualified goodput ≥1.15× best single for the same workload.

A missing measurement, unmatched control, invalid cache state, unverifiable hash, or synthetic record yields `INSUFFICIENT_EVIDENCE`; it is not an implicit pass.

## Invalidation and safety-abort rules

- Prompt mix or cache eligibility changes across topology.
- Client generator cannot sustain declared offered load.
- Warm-up is included in reported measurement.

Safety aborts remain measured events and retain all evidence. Do not exclude an unfavorable but valid run.

## Outputs

- `manifest.json`, `requests.jsonl`, `tokens.jsonl`, telemetry/log channels, and applicable correctness/fault records.
- Derived per-cell statistics with calculation code commit and paired baseline IDs.
- One experiment outcome: `PASS`, `WARN_RETEST`, `FAIL`, or `INSUFFICIENT_EVIDENCE`.

## Interpretation limits

The best closed-loop concurrency is not necessarily safe under burst/open-loop traffic; EXP-009 establishes saturation behavior.

## Research basis

[[SRC-016]](../references/Sources.md#src-016)
