# EXP-007 — Streaming TTFT, ITL, and Latency Decomposition

> **Wiki status:** Proposed · **Evidence state:** D0 — design only · **Last reviewed:** 2026-07-17  
> **Machine-validation status:** Not run. Missing measurements are `INSUFFICIENT_EVIDENCE`, never an implicit pass.


| Field | Value |
|---|---|
| Objective | Measure user-visible first-token and token-to-token latency with a real streaming client, then decompose queue, prefill, decode, and transport where instrumentation permits. |
| Release profiles | All |
| Required evidence | M1/M2 absolute latency SLO block |
| Estimated measured duration | At least 200 successful requests/cell or 10 minutes |
| Risk class | Low |

## Decision question

Do client-observed TTFT and ITL meet p50/p95/p99 SLOs across cache states and operating concurrency, without hiding queue or network time?

## Hypotheses

- **H0:** Latency SLOs fail, timestamps are ambiguous, or client/server decomposition does not reconcile.
- **H1:** Client latency distributions are complete, reproducible, and explainable by measured components.

## Preconditions and provenance

- Use `tools/token_stream_client.py` or an equivalent client that timestamps every non-empty content event on one monotonic clock.
- C2 and C3 state evidence, approved absolute SLOs, and exact client placement are frozen.
- Clock offset ≤5 ms only when cross-host server/client decomposition is reported; client TTFT/ITL do not depend on cross-host clocks.

## Factors, controls, and run order

- Workloads `interactive-128x128`, `interactive-512x128`, and representative long-context cell.
- Cache C2 and C3; topology A, B, dual; concurrency 1 and selected production concurrency.
- At least 20 warm-up requests then 200 successful measured requests/cell; retain failed attempts in the denominator.

## Procedure

1. Prime only the cache state declared by the cell and verify eligibility.
2. Send hashed prompts through the production API path; record send, first content token, every token event, last token, and completion.
3. Collect server request accepted/queued, prefill start/end, first emit, decode events, and response completion when exposed.
4. Collect RTT, retransmits, queue depth, runtime load, and node/link telemetry.
5. Reconcile client E2E with server and network components; flag negative or impossible intervals as instrumentation defects.

## Required measurements

- `LAT-TTFT-C`, `LAT-TTFT-S`, `LAT-ITL`, `LAT-TPOT`, `LAT-E2E`; queue/prefill/first-decode/network components; success/cancel/error rates.

All run-level data validates against [the raw-data schemas](../schemas/README.md). Use the canonical definitions in [Metric Definitions](../wiki/Metric-Definitions.md), the matrix in [`config/benchmark-matrix.yaml`](../config/benchmark-matrix.yaml), and immutable run manifests.

## Acceptance and regression rules

- All approved p95/p99 TTFT and ITL SLOs are met at the production point.
- Candidate p50/p95 latency is not >10% worse and p99 not >15% worse than eligible baseline with confirmed confidence.
- Request success ≥99.99% for the stable aggregate; no failed attempt is removed from the rate.
- No more than 0.1% token events have missing/nonmonotonic timestamps; any clock defect affecting quantiles invalidates the cell.
- C3 gains must be accompanied by recorded cached-token counts and correctness.

A missing measurement, unmatched control, invalid cache state, unverifiable hash, or synthetic record yields `INSUFFICIENT_EVIDENCE`; it is not an implicit pass.

## Invalidation and safety-abort rules

- The client buffers the full response before timestamping tokens.
- Terminal/empty SSE frames are counted as content tokens.
- Client placement or network route changes.

Safety aborts remain measured events and retain all evidence. Do not exclude an unfavorable but valid run.

## Outputs

- `manifest.json`, `requests.jsonl`, `tokens.jsonl`, telemetry/log channels, and applicable correctness/fault records.
- Derived per-cell statistics with calculation code commit and paired baseline IDs.
- One experiment outcome: `PASS`, `WARN_RETEST`, `FAIL`, or `INSUFFICIENT_EVIDENCE`.

## Interpretation limits

Server timing alone is not TTFT as experienced by the caller; client timing is the release authority.

## Research basis

[[SRC-009]](../references/Sources.md#src-009) [[SRC-015]](../references/Sources.md#src-015)
