# EXP-016 — Network and USB4 Fault Injection

> **Wiki status:** Proposed · **Evidence state:** D0 — design only · **Last reviewed:** 2026-07-17  
> **Machine-validation status:** Not run. Missing measurements are `INSUFFICIENT_EVIDENCE`, never an implicit pass.


| Field | Value |
|---|---|
| Objective | Verify explicit failure, bounded detection/recovery, request disposition, and output integrity under controlled delay, loss, rate limit, and link interruption. |
| Release profiles | Availability recovery; mandatory stable for dual operation |
| Required evidence | M2 fault evidence, three repeats/scenario |
| Estimated measured duration | 4–8 hours |
| Risk class | High—intentional connectivity disruption |

## Decision question

Does the coordinator/worker pair fail closed and recover predictably when the USB4 data path degrades or disappears?

## Hypotheses

- **H0:** Network faults cause hangs, silent corruption, indefinite queueing, stale workers, or unbounded recovery.
- **H1:** Every mandatory network fault is detected, contained, and recovered within the approved SLO without corruption or acknowledged-request loss.

## Preconditions and provenance

- Approved fault window, console/out-of-band access, hard timeouts, rollback commands, and healthy fault-free control.
- Use `config/fault-matrix.yaml`; start with software `tc netem`/interface controls before physical disconnect.
- Client records all attempts and acknowledgements; logs/telemetry continue over a separate management path.

## Factors, controls, and run order

- 50 ms ±5 ms delay, 1% loss, 500 Mbit/s rate limit, 10 s interface down; phases idle, prefill, decode as specified.
- Three successful injections per mandatory scenario after one dry rehearsal.
- Production request mix at ≤50% operating load to separate fault behavior from overload.

## Procedure

1. Run a ten-minute fault-free control and critical correctness canary.
2. Schedule the fault against a monotonic timestamp; record planned/actual start/end and exact command/parameters.
3. Inject during the declared request phase while collecting client, runtime, kernel, interface, USB4, and process state.
4. Rollback and verify link/routing/traffic-control state. Measure health, worker registration, service recovery, and post-fault canaries.
5. Repeat three times and compare post-recovery performance to control.

## Required measurements

- Detection latency, in-flight request outcomes, timeout/error semantics, queue behavior, lost acknowledged requests, recovery/rejoin time, link state, output correctness, and post-recovery latency/goodput.

All run-level data validates against [the raw-data schemas](../schemas/README.md). Use the canonical definitions in [Metric Definitions](../wiki/Metric-Definitions.md), the matrix in [`config/benchmark-matrix.yaml`](../config/benchmark-matrix.yaml), and immutable run manifests.

## Acceptance and regression rules

- All mandatory scenarios pass 3/3; no silent corruption or lost acknowledged request.
- Fault detection and service recovery meet approved absolute recovery SLOs.
- No coordinator/worker hang beyond hard timeout; clients receive explicit outcomes.
- Within ten minutes post-recovery, p95 latency/goodput return within 5% of control and critical correctness =100%.
- Traffic-control/interface state is verified clean after every rollback.

A missing measurement, unmatched control, invalid cache state, unverifiable hash, or synthetic record yields `INSUFFICIENT_EVIDENCE`; it is not an implicit pass.

## Invalidation and safety-abort rules

- Fault starts outside the declared phase and is not relabeled.
- Management/logging uses the faulted path and loses decisive evidence.
- Rollback is assumed rather than verified.

Safety aborts remain measured events and retain all evidence. Do not exclude an unfavorable but valid run.

## Outputs

- `manifest.json`, `requests.jsonl`, `tokens.jsonl`, telemetry/log channels, and applicable correctness/fault records.
- Derived per-cell statistics with calculation code commit and paired baseline IDs.
- One experiment outcome: `PASS`, `WARN_RETEST`, `FAIL`, or `INSUFFICIENT_EVIDENCE`.

## Interpretation limits

Software network faults do not fully reproduce retimer/controller/cable failures; physical disconnect is an extended scenario.

## Research basis

[[SRC-007]](../references/Sources.md#src-007) [[SRC-010]](../references/Sources.md#src-010)
