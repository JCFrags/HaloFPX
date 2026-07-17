# EXP-012 — CPU/GPU Utilization and Resource Balance

> **Wiki status:** Proposed · **Evidence state:** D0 — design only · **Last reviewed:** 2026-07-17  
> **Machine-validation status:** Not run. Missing measurements are `INSUFFICIENT_EVIDENCE`, never an implicit pass.


| Field | Value |
|---|---|
| Objective | Attribute bottlenecks and confirm that both nodes, accelerators, memory, CPU, and the link participate as intended without hidden pressure. |
| Release profiles | All |
| Required evidence | Diagnostic evidence attached to performance/soak gates |
| Estimated measured duration | 2–4 hours plus collector controls |
| Risk class | Low |

## Decision question

Are utilization and pressure consistent with the declared partition, and is there enough headroom at the production point?

## Hypotheses

- **H0:** Performance is constrained by an unobserved CPU/GPU/memory/IO bottleneck, worker idleness, swap, or collector distortion.
- **H1:** Resource use is observable, balanced or intentionally asymmetric, and retains declared headroom.

## Preconditions and provenance

- Collector overhead from EXP-001 passes. Use driver/SMU telemetry supported by the exact kernel/ROCm stack; record missing fields explicitly.
- Pin the selected production and knee workload cells from EXP-005–009.

## Factors, controls, and run order

- Topology A, B, dual; prefill-focused, decode-focused, and mixed serving cells.
- Normal 1 Hz collector and short 5–10 Hz diagnostic collector with separate overhead check.
- Production CPU affinity/power profile; one diagnostic affinity control only when investigating.

## Procedure

1. Capture per-core/process utilization/frequency, run queue/context switches, memory/RSS/PSS/faults/swap/PSI, GPU/memory busy/clocks/memory/power/thermal/throttler, disk, network, and runtime queue state.
2. Align samples to request phases and calculate per-node contributions and imbalance indices.
3. Use profiler traces on short representative windows, not the full soak, and record profiler overhead.
4. Compare prefill, decode, and mixed cells to distinguish compute, memory, CPU scheduling, and transport constraints.
5. Inspect journal/kernel logs for reset/retry/throttle events that utilization percentages alone miss.

## Required measurements

- CPU/GPU/memory busy distributions, frequencies, run queue, PSI `some/full`, faults/swap, memory headroom, per-node work share, queue depth, profiler kernels, and collector loss/overhead.

All run-level data validates against [the raw-data schemas](../schemas/README.md). Use the canonical definitions in [Metric Definitions](../wiki/Metric-Definitions.md), the matrix in [`config/benchmark-matrix.yaml`](../config/benchmark-matrix.yaml), and immutable run manifests.

## Acceptance and regression rules

- No swap activity or OOM event in a supported production cell; minimum memory headroom ≥10%.
- PSI `full` sustained time is zero at the production point; recurrent `some` pressure requires attribution and SLO evidence.
- Both nodes show expected work. A worker with <10% median GPU busy while the coordinator is saturated is a blocker unless the frozen partition model predicts it and performance gates still pass.
- No thermal throttle, clock collapse, reset, or profiler/collector overhead beyond EXP-001 limits.
- Any claimed bottleneck is supported by at least two aligned signals, not utilization percentage alone.

A missing measurement, unmatched control, invalid cache state, unverifiable hash, or synthetic record yields `INSUFFICIENT_EVIDENCE`; it is not an implicit pass.

## Invalidation and safety-abort rules

- Telemetry schema/version changes within a block.
- Sample gaps >2 intervals during a short test.
- Profiler is enabled for candidate but not matched baseline.

Safety aborts remain measured events and retain all evidence. Do not exclude an unfavorable but valid run.

## Outputs

- `manifest.json`, `requests.jsonl`, `tokens.jsonl`, telemetry/log channels, and applicable correctness/fault records.
- Derived per-cell statistics with calculation code commit and paired baseline IDs.
- One experiment outcome: `PASS`, `WARN_RETEST`, `FAIL`, or `INSUFFICIENT_EVIDENCE`.

## Interpretation limits

High utilization is neither necessary nor sufficient for good performance; release authority remains SLO-qualified workload results.

## Research basis

[[SRC-013]](../references/Sources.md#src-013) [[SRC-014]](../references/Sources.md#src-014) [[SRC-020]](../references/Sources.md#src-020) [[SRC-021]](../references/Sources.md#src-021)
