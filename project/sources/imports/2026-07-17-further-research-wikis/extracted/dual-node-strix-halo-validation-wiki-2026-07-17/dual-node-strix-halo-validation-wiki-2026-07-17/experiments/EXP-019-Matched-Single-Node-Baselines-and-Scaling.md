# EXP-019 — Matched Single-Node Baselines and Scaling Analysis

> **Wiki status:** Proposed · **Evidence state:** D0 — design only · **Last reviewed:** 2026-07-17  
> **Machine-validation status:** Not run. Missing measurements are `INSUFFICIENT_EVIDENCE`, never an implicit pass.


| Field | Value |
|---|---|
| Objective | Create valid A-only and B-only controls, quantify node asymmetry, and calculate dual-node speedup/efficiency only where a legitimate denominator exists. |
| Release profiles | All; mandatory and nonwaivable |
| Required evidence | M1 controls plus M2/R1 comparison |
| Estimated measured duration | One full matched block plus independent reproduction |
| Risk class | Low |

## Decision question

Does dual-node behavior improve the named workload versus the best eligible single node, or only extend capacity?

## Hypotheses

- **H0:** Baselines are unmatched/stale, nodes differ materially, or the claimed dual benefit is not supported by paired evidence.
- **H1:** A/B controls are reproducible and dual ratios support the selected release-profile claim.

## Preconditions and provenance

- Exact same model, tokenizer, prompts, sampling, context, cache state, runtime commit/build, client path, collectors, power/thermal conditions, and workload cell across A, B, dual.
- Baseline age ≤30 days and no untriaged critical upstream/runtime change between baseline and candidate.

## Factors, controls, and run order

- Topology A, B, dual across primary prefill, decode, latency, throughput, cache, long-context, power, and correctness cells.
- Paired prompt IDs and randomized AB/BA/dual order; two independent run blocks.

## Procedure

1. Run full primary matrix on A and B separately; calculate node asymmetry before using `best_single`.
2. Run the identical matrix on dual in the same block and environment window.
3. Calculate paired ratios and bootstrap confidence intervals for prefill/decode/goodput and latency tails.
4. For capacity-only workload, document measured single-node safe-memory failure and run a smaller same-family control on all topologies.
5. Classify the claim as scale-out, capacity extension, availability recovery, or failure—before publication.

## Required measurements

- A/B asymmetry, dual/best-single ratios, scaling efficiency `(dual/best_single)/2`, absolute SLOs, energy ratio, memory headroom, and correctness equivalence.

All run-level data validates against [the raw-data schemas](../schemas/README.md). Use the canonical definitions in [Metric Definitions](../wiki/Metric-Definitions.md), the matrix in [`config/benchmark-matrix.yaml`](../config/benchmark-matrix.yaml), and immutable run manifests.

## Acceptance and regression rules

- Matched A, B, and dual evidence exists with complete provenance; missing one topology is `INSUFFICIENT_EVIDENCE`.
- Unexplained A/B primary metric difference >5% must be resolved or the weaker node is repaired/rebaselined; both raw baselines remain reported.
- Scale-out stable target: dual goodput ≥1.15× best single; p95 TTFT/ITL ≤1.10× and p99 ≤1.15× best single.
- Capacity extension: large workload is documented unsafe/impossible on one node, meets absolute SLO/memory headroom on dual, and has a smaller matched control; no speedup claim for the large workload.
- Key comparisons reproduce in a second block/day.

A missing measurement, unmatched control, invalid cache state, unverifiable hash, or synthetic record yields `INSUFFICIENT_EVIDENCE`; it is not an implicit pass.

## Invalidation and safety-abort rules

- Single-node uses a smaller model/context/quantization for a scale-out ratio.
- Baseline changes client placement or cache eligibility.
- Only the faster node is reported while hiding the other matched control.

Safety aborts remain measured events and retain all evidence. Do not exclude an unfavorable but valid run.

## Outputs

- `manifest.json`, `requests.jsonl`, `tokens.jsonl`, telemetry/log channels, and applicable correctness/fault records.
- Derived per-cell statistics with calculation code commit and paired baseline IDs.
- One experiment outcome: `PASS`, `WARN_RETEST`, `FAIL`, or `INSUFFICIENT_EVIDENCE`.

## Interpretation limits

Scaling efficiency is descriptive; release value depends on the declared claim and absolute SLOs, not proximity to ideal 2× alone.

## Research basis

[[SRC-006]](../references/Sources.md#src-006) [[SRC-017]](../references/Sources.md#src-017)
