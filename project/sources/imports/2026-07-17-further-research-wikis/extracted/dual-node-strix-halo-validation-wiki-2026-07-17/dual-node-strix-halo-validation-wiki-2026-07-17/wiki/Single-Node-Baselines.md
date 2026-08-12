# Matched Single-Node Baselines

> **Wiki status:** Proposed · **Evidence state:** D0 — design only · **Last reviewed:** 2026-07-17  
> **Machine-validation status:** Not run. Missing measurements are `INSUFFICIENT_EVIDENCE`, never an implicit pass.


## Baseline set

For every dual-node release workload that fits on one node, execute:

- `A-only`: coordinator and accelerator on Node A, USB4 peer disconnected or administratively down.
- `B-only`: same workload on Node B, with the client placed equivalently.
- `dual`: same engine commit/config semantics, model, prompts, output limits, and SLO.

Use the faster of A/B only when a gate explicitly says `best_single`; always retain both to reveal hardware asymmetry.

## Match keys

A baseline is matched only if these fields are identical or explicitly the independent variable: model and shard hashes, quantization, tokenizer, context, KV type, sampling, prompt IDs/token counts, engine commit/build flags, CPU/GPU profile, OS/kernel/driver stack, client location, storage state, cache state, and ambient band.

## Capacity-extension exception

A model/context that cannot safely fit on either single node has no speedup denominator. Required evidence becomes:

1. A documented single-node capacity failure with safe, expected error behavior and complete memory telemetry.
2. A smaller same-family control model that runs A-only, B-only, and dual to quantify transport and partition overhead.
3. Absolute dual-node SLOs and correctness/soak gates for the larger workload.
4. Claim language restricted to capacity enablement.

## Scaling metrics

- Throughput scaling: `dual_goodput / best_single_goodput`.
- Latency overhead: `dual_p95 / best_single_p95 - 1`.
- Energy scaling: `dual_joules_per_token / best_single_joules_per_token` using total two-node wall power.
- Resource balance: per-node compute, memory, link bytes, and idle fraction.

A dual-node configuration that is slower may still be a valid capacity profile, but the release record must say so plainly.
