# EXP-011 — Prefix/KV Cache Efficacy, Eviction, and Isolation

> **Wiki status:** Proposed · **Evidence state:** D0 — design only · **Last reviewed:** 2026-07-17  
> **Machine-validation status:** Not run. Missing measurements are `INSUFFICIENT_EVIDENCE`, never an implicit pass.


| Field | Value |
|---|---|
| Objective | Validate cache accounting, latency benefit, eviction behavior, concurrency effects, and separation between distinct prompts/tenants. |
| Release profiles | All when caching enabled |
| Required evidence | M1/M2 cache release gate |
| Estimated measured duration | 3–6 hours |
| Risk class | Low |

## Decision question

Does the cache return only valid prefix work, at the expected hit rate and performance benefit, without leakage or stale reuse?

## Hypotheses

- **H0:** Cache reports are inaccurate, false hits occur, eviction is unstable, or output/cross-tenant isolation fails.
- **H1:** Eligible exact prefixes hit predictably; ineligible variants miss; eviction and isolation are correct under load.

## Preconditions and provenance

- EXP-004 state controls pass. Cache capacity, keying fields, slot/session policy, persistence, and tenant boundary are documented.
- Prepare exact prefixes and one-token/whitespace/system-prompt/tokenizer variants with known eligibility.

## Factors, controls, and run order

- Prefix lengths 512, 2K, 8K; reuse count 20; C2 no-hit and C3 hit controls.
- Concurrency 1 and production point; cache occupancy 25%, 75%, 100%, and eviction pressure.
- Tenant/session IDs and near-match variants.

## Procedure

1. Prime each exact prefix, then issue suffix variants while recording eligible/cached/processed tokens.
2. Issue near-match/ineligible prompts and verify zero inappropriate cached tokens.
3. Fill cache beyond capacity using distinct prefixes; record deterministic eviction order/behavior and reaccess results.
4. Run concurrent same-prefix and different-prefix traffic at the operating point.
5. Compare outputs and task scores cache-on/off; test restart/persistence only if part of declared product behavior.

## Required measurements

- Prefix token/request hit rate, false-hit rate, evictions, saved prefill ms, time saved/cached token, cache memory, TTFT/goodput, and correctness/isolation outcomes.

All run-level data validates against [the raw-data schemas](../schemas/README.md). Use the canonical definitions in [Metric Definitions](../wiki/Metric-Definitions.md), the matrix in [`config/benchmark-matrix.yaml`](../config/benchmark-matrix.yaml), and immutable run manifests.

## Acceptance and regression rules

- Eligible prefix-token hit rate ≥95% and no more than 2 percentage points below baseline.
- False-hit and cross-tenant/session leakage rate =0; any false reuse is a nonwaivable correctness/security failure.
- Critical correctness cache-on/off =100%; task-quality drift within threshold.
- Cache pressure causes bounded, observable eviction—not unbounded memory or runtime failure.
- Claimed cache benefit is calculated from paired C2/C3 prompts and excludes load/queue differences.

A missing measurement, unmatched control, invalid cache state, unverifiable hash, or synthetic record yields `INSUFFICIENT_EVIDENCE`; it is not an implicit pass.

## Invalidation and safety-abort rules

- Eligibility definition changes after results.
- Prompt canonicalization/tokenization is not hashed.
- Cache occupancy is unknown for an eviction cell.

Safety aborts remain measured events and retain all evidence. Do not exclude an unfavorable but valid run.

## Outputs

- `manifest.json`, `requests.jsonl`, `tokens.jsonl`, telemetry/log channels, and applicable correctness/fault records.
- Derived per-cell statistics with calculation code commit and paired baseline IDs.
- One experiment outcome: `PASS`, `WARN_RETEST`, `FAIL`, or `INSUFFICIENT_EVIDENCE`.

## Interpretation limits

Hit rate from a synthetic reuse distribution must not be presented as production hit rate; production traces require separate privacy-reviewed replay.

## Research basis

[[SRC-009]](../references/Sources.md#src-009)
