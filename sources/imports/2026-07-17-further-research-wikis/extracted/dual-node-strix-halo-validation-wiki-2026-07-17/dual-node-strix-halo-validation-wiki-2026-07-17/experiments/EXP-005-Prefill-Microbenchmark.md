# EXP-005 — Prefill Microbenchmark

> **Wiki status:** Proposed · **Evidence state:** D0 — design only · **Last reviewed:** 2026-07-17  
> **Machine-validation status:** Not run. Missing measurements are `INSUFFICIENT_EVIDENCE`, never an implicit pass.


| Field | Value |
|---|---|
| Objective | Measure uncached prompt-processing throughput and scaling by context length independently of long decode streams and serving queueing. |
| Release profiles | Scale-out and capacity extension |
| Required evidence | Matched M1/M2 performance block |
| Estimated measured duration | 2–6 hours depending on context range |
| Risk class | Low |

## Decision question

How does uncached prefill throughput scale across A, B, and dual, and where do compute, memory, or USB4 limits appear?

## Hypotheses

- **H0:** Dual prefill does not meet the declared profile or exhibits regression, imbalance, or unstable scaling.
- **H1:** Prefill behavior is repeatable, correctly normalized to uncached tokens, and meets absolute/relative gates.

## Preconditions and provenance

- C2 state is proven; prompt caching is disabled or prompts are unique so cached tokens are excluded.
- Use the frozen runtime build and `llama-bench` prompt-processing mode or an equivalent engine timing path validated against client measurements.
- Exact tokenized prompts exist for each length and are identical across topologies.

## Factors, controls, and run order

- Prompt lengths: 128, 512, 2K, 8K, 16K, 32K, and supported higher lengths from the matrix.
- Topology A, B, dual; fixed batch/ubatch, context, split, threads, and power profile.
- Three warm-up iterations and at least ten measured iterations; continue to 30 until 95% CI width ≤5%.

## Procedure

1. Prove C2 and record zero cached prompt tokens.
2. Run prompt-processing-only cells in randomized length/topology blocks; use identical prompt IDs and settings.
3. Collect engine prefill time, client/server timing anchors, CPU/GPU/memory busy, clocks, memory, USB4 bytes, disk I/O, power, and thermals.
4. Repeat the key 2K and 8K cells in an independent block/day.
5. Compute per-length paired dual/best-single ratios and link bytes per uncached prompt token.

## Required measurements

- `PREFILL-TPS`, prefill latency, scaling ratio/efficiency, link bytes/token, utilization, energy/prompt token, memory headroom, and repeatability.

All run-level data validates against [the raw-data schemas](../schemas/README.md). Use the canonical definitions in [Metric Definitions](../wiki/Metric-Definitions.md), the matrix in [`config/benchmark-matrix.yaml`](../config/benchmark-matrix.yaml), and immutable run manifests.

## Acceptance and regression rules

- No cached tokens enter the numerator; otherwise the cell is invalid.
- Candidate prefill throughput is not >5% below its eligible same-topology baseline with confirmed confidence.
- All declared interactive/long-context absolute prefill or TTFT SLOs remain achievable at their operating point.
- No crash, OOM below declared supported limit, thermal throttle, GPU reset, or unexplained worker inactivity.
- Scale-out publication uses same-workload paired ratios; capacity-only cells are not assigned a speedup.

A missing measurement, unmatched control, invalid cache state, unverifiable hash, or synthetic record yields `INSUFFICIENT_EVIDENCE`; it is not an implicit pass.

## Invalidation and safety-abort rules

- Prompt token count differs across topology.
- Batch/ubatch/context/split changes inside a paired block.
- Cached prompt tokens are nonzero.

Safety aborts remain measured events and retain all evidence. Do not exclude an unfavorable but valid run.

## Outputs

- `manifest.json`, `requests.jsonl`, `tokens.jsonl`, telemetry/log channels, and applicable correctness/fault records.
- Derived per-cell statistics with calculation code commit and paired baseline IDs.
- One experiment outcome: `PASS`, `WARN_RETEST`, `FAIL`, or `INSUFFICIENT_EVIDENCE`.

## Interpretation limits

Prompt-processing microbenchmarks exclude scheduler queueing and do not substitute for client TTFT.

## Research basis

[[SRC-008]](../references/Sources.md#src-008)
