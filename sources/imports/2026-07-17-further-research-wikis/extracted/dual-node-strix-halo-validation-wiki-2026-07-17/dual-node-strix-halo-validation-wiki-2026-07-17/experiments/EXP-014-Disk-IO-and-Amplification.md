# EXP-014 — Disk I/O and Amplification

> **Wiki status:** Proposed · **Evidence state:** D0 — design only · **Last reviewed:** 2026-07-17  
> **Machine-validation status:** Not run. Missing measurements are `INSUFFICIENT_EVIDENCE`, never an implicit pass.


| Field | Value |
|---|---|
| Objective | Quantify physical reads/writes attributable to model startup, page-cache state, runtime caches, logging, and long operation. |
| Release profiles | All |
| Required evidence | M1/M2 storage gate |
| Estimated measured duration | 2–4 hours plus cold boots |
| Risk class | Low |

## Decision question

Does the runtime read/write only the expected data, and are cold/warm amplification and sustained write rates bounded?

## Hypotheses

- **H0:** Physical I/O is excessive, misattributed, state-dependent in an unexplained way, or risks storage wear/capacity.
- **H1:** Cold and warm amplification are measured with defensible denominators and writes remain bounded.

## Preconditions and provenance

- Known model artifact byte size and filesystem/container layout; record compression, reflink, encryption, network mounts, and page-cache method.
- Use cgroup-v2 `io.stat` for runtime attribution where possible, with block-device counters and major faults as cross-checks.

## Factors, controls, and run order

- C0 cold load, C1 OS-warm load, C2 steady inference, C3 cache-hit inference, and 1-hour mixed load.
- Topology A, B, dual; logging normal and a separately labeled reduced-log diagnostic only if needed.

## Procedure

1. Record starting cgroup/device counters, model logical/allocated bytes, filesystem free space, and mount options.
2. Execute EXP-003/004 state transitions and capture ending counters, page faults, per-process/cgroup I/O, and model/cache/log file changes.
3. Run one hour at the production mix and inventory bytes written by path where feasible.
4. Reconcile cgroup and device deltas, documenting shared-device noise and confidence.
5. Compute amplification only when the logical-byte denominator is valid; otherwise report absolute attributable bytes.

## Required measurements

- Cold/warm read amplification, write amplification/absolute writes, read/write bytes and IOPS, latency, major faults, cache/log growth, free-space slope, and data-file hashes.

All run-level data validates against [the raw-data schemas](../schemas/README.md). Use the canonical definitions in [Metric Definitions](../wiki/Metric-Definitions.md), the matrix in [`config/benchmark-matrix.yaml`](../config/benchmark-matrix.yaml), and immutable run manifests.

## Acceptance and regression rules

- Cold read amplification ≤1.25× logical model bytes and warm read amplification ≤0.10×.
- No unexpected full model reread during C2/C3 steady operation.
- No unbounded cache/log/temp-file growth; projected free-space exhaustion exceeds the approved maintenance interval with ≥2× margin.
- Candidate I/O amplification remains inside configured regression thresholds.
- Any shared-device noise large enough to change the gate result yields `INSUFFICIENT_EVIDENCE` and a rerun on an attributable setup.

A missing measurement, unmatched control, invalid cache state, unverifiable hash, or synthetic record yields `INSUFFICIENT_EVIDENCE`; it is not an implicit pass.

## Invalidation and safety-abort rules

- Counters wrap/reset without reconciliation.
- Another high-I/O job uses the same device.
- Logical bytes are used despite transparent compression/dedup without documenting denominator impact.

Safety aborts remain measured events and retain all evidence. Do not exclude an unfavorable but valid run.

## Outputs

- `manifest.json`, `requests.jsonl`, `tokens.jsonl`, telemetry/log channels, and applicable correctness/fault records.
- Derived per-cell statistics with calculation code commit and paired baseline IDs.
- One experiment outcome: `PASS`, `WARN_RETEST`, `FAIL`, or `INSUFFICIENT_EVIDENCE`.

## Interpretation limits

Block counters show physical I/O behavior but not NAND-level write amplification inside the SSD controller unless vendor telemetry provides it.

## Research basis

[[SRC-019]](../references/Sources.md#src-019) [[SRC-020]](../references/Sources.md#src-020)
