# EXP-015 — Output Correctness, Protocol Integrity, and Task Quality

> **Wiki status:** Proposed · **Evidence state:** D0 — design only · **Last reviewed:** 2026-07-17  
> **Machine-validation status:** Not run. Missing measurements are `INSUFFICIENT_EVIDENCE`, never an implicit pass.


| Field | Value |
|---|---|
| Objective | Verify deterministic/protocol outputs, cache/topology equivalence, structured-output validity, task quality, and absence of silent corruption. |
| Release profiles | All |
| Required evidence | Nonwaivable M1/M2 correctness gate |
| Estimated measured duration | 2–12 hours depending on suite |
| Risk class | Low |

## Decision question

Does dual-node execution produce valid, noncorrupt outputs with quality equivalent to approved single-node controls across cache and context conditions?

## Hypotheses

- **H0:** Outputs are malformed, silently corrupted, wrongly cached, or materially worse than baseline.
- **H1:** Critical correctness is perfect and statistical task quality remains within the declared drift bound.

## Preconditions and provenance

- Freeze prompt suite, expected outputs/invariants, evaluator code/commit, tokenizer, normalization, sampling, and licenses.
- Separate deterministic/protocol tests from stochastic task-quality tests. Do not use a subjective judge as the only corruption detector.
- Include malformed-input, cancellation, stop-token, UTF-8, JSON/grammar, context-position, and cache near-match cases.

## Factors, controls, and run order

- Topology A, B, dual; C2 and C3; short, medium, and maximum-supported context.
- Deterministic suite at temperature 0/fixed seed; task suite at predeclared sampling and repetitions.
- At least one cross-topology paired output set and one independent reproduction.

## Procedure

1. Run exact/token-exact protocol canaries and validate status, finish reason, token count, stream framing, UTF-8, and output hash.
2. Run JSON/grammar/schema and invariant tests, including malformed and boundary inputs.
3. Run context-position retrieval and cache exact/near-match tests.
4. Execute the declared task-quality suite on A, B, and dual with blind/randomized identifiers.
5. Classify discrepancies as expected numerical variation, quality drift, protocol defect, or silent corruption; retain original outputs/hashes.

## Required measurements

- Critical pass rate, schema/parse rate, exact/token agreement, task scores and confidence intervals, topology/cache deltas, malformed/truncated/duplicate output count, and adjudications.

All run-level data validates against [the raw-data schemas](../schemas/README.md). Use the canonical definitions in [Metric Definitions](../wiki/Metric-Definitions.md), the matrix in [`config/benchmark-matrix.yaml`](../config/benchmark-matrix.yaml), and immutable run manifests.

## Acceptance and regression rules

- Critical deterministic/protocol correctness rate =100%; silent corruption =0.
- Every successful response parses/frames according to the declared API; no missing, duplicated, or out-of-order content events.
- Task-quality delta remains inside the approved suite-specific threshold with its confidence rule.
- Cache-on/off and A/B/dual discrepancies are either zero for critical invariants or explicitly within the predeclared numerical tolerance.
- Any critical failure blocks stable release and is not waivable.

A missing measurement, unmatched control, invalid cache state, unverifiable hash, or synthetic record yields `INSUFFICIENT_EVIDENCE`; it is not an implicit pass.

## Invalidation and safety-abort rules

- Evaluator version or expected output changes after candidate results are seen.
- Prompts leak topology labels to a subjective evaluator.
- Only aggregate score is retained without per-item evidence.

Safety aborts remain measured events and retain all evidence. Do not exclude an unfavorable but valid run.

## Outputs

- `manifest.json`, `requests.jsonl`, `tokens.jsonl`, telemetry/log channels, and applicable correctness/fault records.
- Derived per-cell statistics with calculation code commit and paired baseline IDs.
- One experiment outcome: `PASS`, `WARN_RETEST`, `FAIL`, or `INSUFFICIENT_EVIDENCE`.

## Interpretation limits

Passing this suite supports only its declared tasks/languages/input domain; it is not a universal model-quality claim.

## Research basis

[[SRC-009]](../references/Sources.md#src-009) [[SRC-018]](../references/Sources.md#src-018)
