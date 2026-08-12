# EXP-010 — Long-Context Scaling and Safe Limits

> **Wiki status:** Proposed · **Evidence state:** D0 — design only · **Last reviewed:** 2026-07-17  
> **Machine-validation status:** Not run. Missing measurements are `INSUFFICIENT_EVIDENCE`, never an implicit pass.


| Field | Value |
|---|---|
| Objective | Measure latency, memory/KV growth, link traffic, correctness, and safe failure behavior from short context through the declared limit. |
| Release profiles | All; mandatory for capacity extension |
| Required evidence | M1/M2 context envelope |
| Estimated measured duration | 4–12 hours/model depending on maximum context |
| Risk class | Medium—OOM/thermal boundary probes |

## Decision question

What context length is correct and operationally safe on each topology, and does the dual-node boundary extend capacity without hidden quality or stability loss?

## Hypotheses

- **H0:** Context scaling is nonlinear/unexplained, correctness degrades, memory headroom is insufficient, or failures are unsafe.
- **H1:** A measured supported limit and performance slope are established with correct retrieval behavior and bounded failure beyond the limit.

## Preconditions and provenance

- Tokenizer-generated exact-length corpus with content hashes and position-sensitive retrieval probes at beginning/middle/end.
- Declared model context, KV format, RoPE/scaling settings, batch/ubatch, split, and memory policy are frozen.
- Safety stop before host OOM cascade, swap storm, filesystem damage, or thermal limit.

## Factors, controls, and run order

- Lengths: powers of two from 2K to supported limit, plus 75% and 100% of declared limit; five requests/length minimum.
- Topology A, B, dual; C2 primary and C3 selected prefix cells.
- Output 32–128 tokens, deterministic probes plus a task-quality subset.

## Procedure

1. Run shortest-to-longer within a block for safety, then reproduce selected lengths in reverse/randomized order to detect hysteresis.
2. At each length, verify actual prompt tokens, run retrieval/invariant probes, and collect prefill/decode/TTFT/ITL, memory, faults, link, disk, power, and thermal data.
3. Record allocation failures and API errors as results; do not discard a cleanly rejected over-limit request.
4. Identify latency/memory slopes and the first knee/failure. Repeat 50%, 75%, and supported maximum independently.
5. Document single-node infeasibility using measured safe-capacity evidence for any capacity-extension claim.

## Required measurements

- Prefill latency/TPS by length, decode TPS/ITL, E2E, peak/resident memory, KV bytes/token, USB4 bytes/token, faults, energy, thermal margin, retrieval accuracy, and failure mode.

All run-level data validates against [the raw-data schemas](../schemas/README.md). Use the canonical definitions in [Metric Definitions](../wiki/Metric-Definitions.md), the matrix in [`config/benchmark-matrix.yaml`](../config/benchmark-matrix.yaml), and immutable run manifests.

## Acceptance and regression rules

- Critical position/invariant correctness =100% through the advertised supported limit.
- Memory headroom ≥10% at the supported operating limit; no swap or OOM-killer event.
- Long-context latency/slope is not >15% worse than eligible baseline with confirmed confidence.
- No crash/reset/oops/throttle. Requests above the safe limit fail explicitly and leave service healthy.
- Capacity-extension claim includes measured evidence that the large cell cannot meet safe single-node memory/headroom requirements, plus a smaller matched control.

A missing measurement, unmatched control, invalid cache state, unverifiable hash, or synthetic record yields `INSUFFICIENT_EVIDENCE`; it is not an implicit pass.

## Invalidation and safety-abort rules

- Actual token length differs from labeled length.
- Context/RoPE/KV settings differ across topology.
- OOM leads to missing logs because evidence storage shares exhausted memory/disk.

Safety aborts remain measured events and retain all evidence. Do not exclude an unfavorable but valid run.

## Outputs

- `manifest.json`, `requests.jsonl`, `tokens.jsonl`, telemetry/log channels, and applicable correctness/fault records.
- Derived per-cell statistics with calculation code commit and paired baseline IDs.
- One experiment outcome: `PASS`, `WARN_RETEST`, `FAIL`, or `INSUFFICIENT_EVIDENCE`.

## Interpretation limits

Passing synthetic needle retrieval does not establish broad model quality; pair it with the declared task suite.

## Research basis

[[SRC-009]](../references/Sources.md#src-009)
