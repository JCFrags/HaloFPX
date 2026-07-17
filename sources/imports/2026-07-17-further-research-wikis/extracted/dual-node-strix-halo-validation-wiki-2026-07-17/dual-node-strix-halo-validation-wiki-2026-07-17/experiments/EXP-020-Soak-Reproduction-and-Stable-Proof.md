# EXP-020 — Soak, Reproduction, and Stable Proof

> **Wiki status:** Proposed · **Evidence state:** D0 — design only · **Last reviewed:** 2026-07-17  
> **Machine-validation status:** Not run. Missing measurements are `INSUFFICIENT_EVIDENCE`, never an implicit pass.


| Field | Value |
|---|---|
| Objective | Demonstrate sustained correctness, availability, telemetry continuity, thermal/link stability, and independent reproducibility before stable acceptance. |
| Release profiles | All |
| Required evidence | G3 24-hour RC and G4 72-hour R1 proof |
| Estimated measured duration | 24-hour RC plus independent 72-hour stable soak |
| Risk class | Medium—long unattended operation with safety automation |

## Decision question

Can the exact release candidate run the production mix for 72 hours with no hard events, SLO drift, evidence gaps, or upstream blockers, and reproduce key results independently?

## Hypotheses

- **H0:** Long operation reveals instability, thermal/link drift, resource leakage, correctness loss, telemetry gaps, or nonreproducible performance.
- **H1:** The candidate maintains its declared SLO/correctness envelope for 72 hours and key blocks reproduce.

## Preconditions and provenance

- All EXP-001–019 required cells pass or have an allowed, unexpired waiver; candidate artifacts and configuration are frozen.
- Upstream ledger is fresh at start; no P0/P1 blocker. Safety watchdogs can abort on thermal/storage/memory limits while preserving evidence.
- Mixed workload weights, daily canaries, fault-free policy, maintenance exclusions, and evidence-gap rules are predeclared.

## Factors, controls, and run order

- 24-hour RC soak after integration, then a separate 72-hour stable soak after remediation/freeze.
- Mix short interactive, medium prefill, long context, C2/C3 cache, and periodic concurrency bursts at ≤80% saturation limit.
- Key 2K prefill, 256-token decode, interactive latency, and correctness blocks repeated before and after soak.

## Procedure

1. Capture full provenance/upstream snapshot and run pre-soak baseline/correctness block.
2. Run the frozen mix continuously. Sample telemetry at 1 Hz, logs continuously, link state on events, and critical correctness canaries at least hourly.
3. Record all restarts, errors, retries, cancellations, maintenance, time-sync changes, and evidence gaps; no undocumented intervention.
4. At completion, rerun key baseline cells, verify raw hashes, compare start/end performance/resource state, and refresh upstream watch.
5. Execute an independent reproduction block/day and assemble the signed release summary/decision.

## Required measurements

- Hours, request/token counts, success and critical correctness, p50/p95/p99 latency by time window, goodput, memory/process/file growth, USB4 state, retransmits, power/thermal/throttle, hard events, evidence gaps, and upstream freshness.

All run-level data validates against [the raw-data schemas](../schemas/README.md). Use the canonical definitions in [Metric Definitions](../wiki/Metric-Definitions.md), the matrix in [`config/benchmark-matrix.yaml`](../config/benchmark-matrix.yaml), and immutable run manifests.

## Acceptance and regression rules

- G3: 24 hours complete. G4: ≥72 measured hours with no raw-evidence gap >60 s.
- Unexpected crash, hang, kernel oops, GPU reset, silent corruption, thermal throttle, or normal-run USB4 renegotiation =0.
- Request success ≥99.99%; critical correctness =100%; all approved absolute SLOs met in every declared reporting window.
- No unbounded memory/process/cache/log growth; end-of-soak key metrics remain within regression thresholds.
- At least two independent reproduction blocks and fresh upstream ledger with zero untriaged P0/P1 blockers.

A missing measurement, unmatched control, invalid cache state, unverifiable hash, or synthetic record yields `INSUFFICIENT_EVIDENCE`; it is not an implicit pass.

## Invalidation and safety-abort rules

- Candidate software/config/model changes during the soak.
- Telemetry gap >60 s is backfilled or interpolated and treated as observed evidence.
- An unexpected restart is called maintenance after the fact.

Safety aborts remain measured events and retain all evidence. Do not exclude an unfavorable but valid run.

## Outputs

- `manifest.json`, `requests.jsonl`, `tokens.jsonl`, telemetry/log channels, and applicable correctness/fault records.
- Derived per-cell statistics with calculation code commit and paired baseline IDs.
- One experiment outcome: `PASS`, `WARN_RETEST`, `FAIL`, or `INSUFFICIENT_EVIDENCE`.

## Interpretation limits

A 72-hour pass supports the exact candidate/testbed/profile and workload envelope; it is not a lifetime-reliability or all-environment guarantee.

## Research basis

[[SRC-022]](../references/Sources.md#src-022) [[SRC-024]](../references/Sources.md#src-024)
