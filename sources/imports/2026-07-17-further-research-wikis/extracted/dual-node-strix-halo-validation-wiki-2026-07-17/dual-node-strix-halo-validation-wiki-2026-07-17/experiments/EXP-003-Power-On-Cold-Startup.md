# EXP-003 — Power-On-Cold Startup and Model Load

> **Wiki status:** Proposed · **Evidence state:** D0 — design only · **Last reviewed:** 2026-07-17  
> **Machine-validation status:** Not run. Missing measurements are `INSUFFICIENT_EVIDENCE`, never an implicit pass.


| Field | Value |
|---|---|
| Objective | Measure boot-to-service and launch-to-ready behavior when neither node, process, nor OS page cache has retained model state. |
| Release profiles | All |
| Required evidence | M1 and M2 cold-start evidence |
| Estimated measured duration | At least 7 controlled boots per topology |
| Risk class | Medium—reboots and service interruption |

## Decision question

Are cold startup, physical reads, first-request latency, energy, and correctness repeatable and within the declared operational SLO?

## Hypotheses

- **H0:** Cold state is not proven, startup is unstable, or load/read behavior breaches the release envelope.
- **H1:** Each topology reaches a correct ready state from verified C0 with bounded load time and disk amplification.

## Preconditions and provenance

- EXP-001 passes; automatic service startup is disabled unless it is part of the measured path.
- C0 requires a controlled power cycle/reboot with evidence that the runtime was not preloaded. `drop_caches` is not the primary C0 method.
- Model storage, filesystem, encryption/compression, firmware, and boot target are frozen.

## Factors, controls, and run order

- Topology: Node A, Node B, dual. Counterbalance topology/boot order across days.
- Measure both `boot complete → service ready` and `runtime launch → ready`, plus first canary TTFT.
- Seven valid boots per topology; continue if cold-start MAD/median >10%.

## Procedure

1. Before reboot, finalize the prior run and verify the next boot marker/run ID.
2. Reboot/power cycle; capture boot ID, journal start, memory/page-cache evidence, and service launch timestamp.
3. Start worker/coordinator using the frozen commands. Poll readiness through the same endpoint used in production.
4. Issue one deterministic `smoke-64x16` canary immediately after readiness; do not prime first.
5. Collect block reads, major faults, wall power, CPU/GPU activity, USB4 traffic, temperatures, logs, and output correctness.
6. Repeat until the minimum and confidence rule are satisfied.

## Required measurements

- Boot-to-ready, launch-to-listen, launch-to-model-ready, first-canary TTFT/E2E, cold read amplification, major faults, energy-to-ready, peak memory, and errors.

All run-level data validates against [the raw-data schemas](../schemas/README.md). Use the canonical definitions in [Metric Definitions](../wiki/Metric-Definitions.md), the matrix in [`config/benchmark-matrix.yaml`](../config/benchmark-matrix.yaml), and immutable run manifests.

## Acceptance and regression rules

- Every valid boot reaches a correct ready state without manual repair, crash, oops, GPU reset, or stale worker registration.
- Cold model-load p95 meets the approved absolute SLO and is not >10% slower than the eligible baseline.
- `IO-READ-AMP-COLD` ≤1.25× unique logical model bytes unless a reviewed filesystem/container explanation has a stricter alternative denominator.
- First canary critical correctness =100%; no request is accepted before the runtime is truly ready.
- Cold-start repeatability MAD/median ≤10%.

A missing measurement, unmatched control, invalid cache state, unverifiable hash, or synthetic record yields `INSUFFICIENT_EVIDENCE`; it is not an implicit pass.

## Invalidation and safety-abort rules

- The model was read or runtime launched before the measured start.
- Boot did not use the frozen kernel/firmware/profile.
- An operator manually primes the service before the canary.

Safety aborts remain measured events and retain all evidence. Do not exclude an unfavorable but valid run.

## Outputs

- `manifest.json`, `requests.jsonl`, `tokens.jsonl`, telemetry/log channels, and applicable correctness/fault records.
- Derived per-cell statistics with calculation code commit and paired baseline IDs.
- One experiment outcome: `PASS`, `WARN_RETEST`, `FAIL`, or `INSUFFICIENT_EVIDENCE`.

## Interpretation limits

Cold startup results do not represent steady-state latency or exact-prefix cache performance.

## Research basis

[[SRC-019]](../references/Sources.md#src-019)
