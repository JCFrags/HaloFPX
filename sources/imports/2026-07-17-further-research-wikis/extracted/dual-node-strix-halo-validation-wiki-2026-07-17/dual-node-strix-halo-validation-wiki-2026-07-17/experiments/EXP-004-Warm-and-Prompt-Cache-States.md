# EXP-004 — Warm and Prompt-Cache State Trials

> **Wiki status:** Proposed · **Evidence state:** D0 — design only · **Last reviewed:** 2026-07-17  
> **Machine-validation status:** Not run. Missing measurements are `INSUFFICIENT_EVIDENCE`, never an implicit pass.


| Field | Value |
|---|---|
| Objective | Disentangle OS page-cache warmth, resident model state, and exact-prefix/KV reuse, then quantify the benefit and correctness of each. |
| Release profiles | All |
| Required evidence | M1/M2 cache-state controls |
| Estimated measured duration | 2–4 hours per topology |
| Risk class | Low |

## Decision question

Can C1, C2, and C3 states be proven independently, and do they produce expected I/O/cache behavior without hidden correctness changes?

## Hypotheses

- **H0:** Cache states are conflated, unproven, or exhibit false hits, excessive warm reads, or correctness divergence.
- **H1:** Each state has direct evidence and produces distinct, repeatable behavior.

## Preconditions and provenance

- EXP-003 establishes C0 behavior. A cache-state transition log and exact prompt/token hashes exist.
- C1: OS page cache primed, runtime stopped, no model-resident process. C2: runtime/model resident, prompt/KV reusable state cleared or unique prompt used. C3: identical eligible prefix deliberately primed.
- Runtime cache controls and slot/session behavior are documented; eviction is explicit rather than assumed.

## Factors, controls, and run order

- Cache states C1, C2, C3; topologies A, B, dual; prompt lengths 512, 2K, and 8K prefix cells.
- Randomize C2/C3 request order within blocks while re-establishing state before each request.
- At least 20 measured requests/cell after two primes where the state definition permits.

## Procedure

1. Establish and record C1, launch the runtime, then issue a unique deterministic request while collecting block reads and faults.
2. Establish C2 with the model resident and no eligible prefix; issue hashed unique prompts and verify reported cached tokens are zero.
3. Establish C3 by priming the exact prefix, then vary only the declared suffix; record eligible, cached, and processed tokens.
4. Repeat cache-on and cache-off correctness comparisons using identical sampling controls.
5. Force one eviction/slot-reuse cycle and verify the state transition is visible rather than silently counted as a hit.

## Required measurements

- Model-load time, warm read amplification, TTFT/prefill time, eligible/cached/processed prompt tokens, token-hit and request-hit rates, saved prefill time, and output correctness.

All run-level data validates against [the raw-data schemas](../schemas/README.md). Use the canonical definitions in [Metric Definitions](../wiki/Metric-Definitions.md), the matrix in [`config/benchmark-matrix.yaml`](../config/benchmark-matrix.yaml), and immutable run manifests.

## Acceptance and regression rules

- C1 warm read amplification ≤0.10× logical model bytes and no unexpected full model reread.
- C2 cached prompt tokens =0 for prompts declared ineligible; any false hit is a correctness failure.
- C3 eligible prefix-token hit rate ≥95% and not more than 2 percentage points below baseline.
- Cache-on versus cache-off critical correctness =100%; task-quality drift remains inside its suite threshold.
- Every state has direct state evidence; a fast result without state proof is invalid, not a pass.

A missing measurement, unmatched control, invalid cache state, unverifiable hash, or synthetic record yields `INSUFFICIENT_EVIDENCE`; it is not an implicit pass.

## Invalidation and safety-abort rules

- Prompt/token hashes differ between a matched cache comparison.
- A previous request remains eligible in a C2 cell.
- The runtime reloads or restarts without the run being reclassified.

Safety aborts remain measured events and retain all evidence. Do not exclude an unfavorable but valid run.

## Outputs

- `manifest.json`, `requests.jsonl`, `tokens.jsonl`, telemetry/log channels, and applicable correctness/fault records.
- Derived per-cell statistics with calculation code commit and paired baseline IDs.
- One experiment outcome: `PASS`, `WARN_RETEST`, `FAIL`, or `INSUFFICIENT_EVIDENCE`.

## Interpretation limits

A cache hit-rate result applies only to the declared cache implementation, capacity, prompt identity rules, and concurrency.

## Research basis

[[SRC-009]](../references/Sources.md#src-009) [[SRC-019]](../references/Sources.md#src-019)
