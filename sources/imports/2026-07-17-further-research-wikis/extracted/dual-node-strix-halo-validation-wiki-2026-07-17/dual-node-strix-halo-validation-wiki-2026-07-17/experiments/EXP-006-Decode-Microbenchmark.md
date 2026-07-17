# EXP-006 — Decode Microbenchmark

> **Wiki status:** Proposed · **Evidence state:** D0 — design only · **Last reviewed:** 2026-07-17  
> **Machine-validation status:** Not run. Missing measurements are `INSUFFICIENT_EVIDENCE`, never an implicit pass.


| Field | Value |
|---|---|
| Objective | Measure steady single-stream token generation independently of prefill and serving concurrency. |
| Release profiles | Scale-out and capacity extension |
| Required evidence | Matched M1/M2 performance block |
| Estimated measured duration | 2–4 hours |
| Risk class | Low |

## Decision question

What is the steady decode rate/TPOT for the exact model and topology, and does dual partitioning improve or degrade it?

## Hypotheses

- **H0:** Decode is unstable, regressed, incorrect, or dominated by avoidable transport/scheduling overhead.
- **H1:** Decode rate is repeatable and meets the selected profile without correctness or stability defects.

## Preconditions and provenance

- C2 model-resident state; deterministic sampling; fixed context/batch/split and output length.
- Use `llama-bench` text-generation mode or a validated equivalent; retain raw client streaming trials in EXP-007.

## Factors, controls, and run order

- Output lengths 128, 256, and 512 after short and representative context prefixes.
- Topology A, B, dual; concurrency=1 for the primary microbenchmark.
- Three warm-ups; at least ten measured iterations/cell until confidence rule.

## Procedure

1. Establish C2 and run a deterministic canary.
2. Run text-generation-only cells with fixed seed and output limits; randomize topology order.
3. Collect generation time, output token count, CPU/GPU/memory busy, clocks, USB4 payload, power, temperature, and output hashes.
4. Calculate per-request TPOT/decode TPS and paired topology ratios.
5. Reproduce the 256-token primary cell independently.

## Required measurements

- `DECODE-TPS`, `LAT-TPOT`, link bytes/output token, energy/output token, utilization balance, memory headroom, and output correctness.

All run-level data validates against [the raw-data schemas](../schemas/README.md). Use the canonical definitions in [Metric Definitions](../wiki/Metric-Definitions.md), the matrix in [`config/benchmark-matrix.yaml`](../config/benchmark-matrix.yaml), and immutable run manifests.

## Acceptance and regression rules

- Candidate decode TPS is not >5% below its eligible baseline with confirmed confidence.
- Critical deterministic canary correctness =100%; no malformed/duplicate/skipped stream content.
- No hard event or throttle; repeatability MAD/median ≤5%.
- For scale-out claims, publish dual/best-single for the same workload and note when single-stream decode slows despite higher aggregate capacity.

A missing measurement, unmatched control, invalid cache state, unverifiable hash, or synthetic record yields `INSUFFICIENT_EVIDENCE`; it is not an implicit pass.

## Invalidation and safety-abort rules

- Output token count or stop rule differs across paired runs.
- Prompt/KV cache eligibility differs.
- Runtime silently retries a failed generation without recording the attempt.

Safety aborts remain measured events and retain all evidence. Do not exclude an unfavorable but valid run.

## Outputs

- `manifest.json`, `requests.jsonl`, `tokens.jsonl`, telemetry/log channels, and applicable correctness/fault records.
- Derived per-cell statistics with calculation code commit and paired baseline IDs.
- One experiment outcome: `PASS`, `WARN_RETEST`, `FAIL`, or `INSUFFICIENT_EVIDENCE`.

## Interpretation limits

Single-stream decode rate does not predict aggregate serving goodput; use EXP-008/009 for concurrency.

## Research basis

[[SRC-008]](../references/Sources.md#src-008)
