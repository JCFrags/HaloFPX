# EXP-013 — Power, Thermals, and Energy Efficiency

> **Wiki status:** Proposed · **Evidence state:** D0 — design only · **Last reviewed:** 2026-07-17  
> **Machine-validation status:** Not run. Missing measurements are `INSUFFICIENT_EVIDENCE`, never an implicit pass.


| Field | Value |
|---|---|
| Objective | Measure gross two-node wall energy, per-node diagnostic power, thermal steady state, margin, throttling, and energy per token. |
| Release profiles | All |
| Required evidence | M1/M2 efficiency and safety gate |
| Estimated measured duration | 30-minute steady state per cell plus soak coverage |
| Risk class | Medium—thermal stress within vendor limits |

## Decision question

Does the system sustain the production workload without throttling and within its power/thermal/energy envelope?

## Hypotheses

- **H0:** Power/thermal behavior is unmeasured, throttled, unsafe, or materially less efficient than baseline.
- **H1:** Gross energy and thermal margin are repeatable, safe, and within regression thresholds.

## Preconditions and provenance

- Calibrated or characterized wall meter(s) at ≥1 Hz preferred; driver-reported power is retained as diagnostic, not substituted silently for wall power.
- Ambient sensor location and target ±2 °C are frozen. Cooling, fan policy, chassis orientation, and power profile are documented.

## Factors, controls, and run order

- Idle, cold load, prefill, decode, production mixed load, knee load, and selected long-context cell.
- Topology A, B, dual; at least 30 minutes after temperatures reach a stable trend.

## Procedure

1. Zero/check meter clocks and record idle baseline for ten minutes.
2. Run each cell while collecting wall watts, driver power, temperatures/hotspot if exposed, clocks, throttler indicators, fan, ambient, and token/request counters.
3. Integrate gross joules for the full measured interval; report idle-subtracted energy only as a secondary metric.
4. Continue the production mix until temperature slope is near steady state or at least 30 minutes; include 72-hour soak telemetry in final review.
5. Correlate any clock/power reduction with temperature and performance.

## Required measurements

- Gross/idle watts, joules/run, joules/prompt token, joules/output token, peak/steady temperature, thermal margin, throttle count/duration, clocks, ambient, and performance per watt.

All run-level data validates against [the raw-data schemas](../schemas/README.md). Use the canonical definitions in [Metric Definitions](../wiki/Metric-Definitions.md), the matrix in [`config/benchmark-matrix.yaml`](../config/benchmark-matrix.yaml), and immutable run manifests.

## Acceptance and regression rules

- Thermal throttle events =0 in normal release runs and soak.
- Minimum exposed thermal margin ≥5 °C at the production point; <10 °C is a warning requiring ambient/cooling review.
- Energy/output token is not >7% worse than eligible dual-node baseline; >4% is warn/retest.
- Ambient remains within target ±2 °C and no undocumented fan/power-policy transition occurs.
- Power/thermal logs cover the entire soak with no evidence gap >60 s.

A missing measurement, unmatched control, invalid cache state, unverifiable hash, or synthetic record yields `INSUFFICIENT_EVIDENCE`; it is not an implicit pass.

## Invalidation and safety-abort rules

- Wall meter clock cannot be aligned to the run.
- Ambient/cooling configuration changes.
- Only driver power is reported while the claim says wall/system energy.

Safety aborts remain measured events and retain all evidence. Do not exclude an unfavorable but valid run.

## Outputs

- `manifest.json`, `requests.jsonl`, `tokens.jsonl`, telemetry/log channels, and applicable correctness/fault records.
- Derived per-cell statistics with calculation code commit and paired baseline IDs.
- One experiment outcome: `PASS`, `WARN_RETEST`, `FAIL`, or `INSUFFICIENT_EVIDENCE`.

## Interpretation limits

Energy efficiency depends on workload mix and utilization; do not compare unmatched prompt/output ratios.

## Research basis

[[SRC-013]](../references/Sources.md#src-013) [[SRC-029]](../references/Sources.md#src-029)
