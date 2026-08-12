---
section_id: "34"
title: "MoE procedures and checks"
status: "needs-machine-validation"
last_verified: "2026-07-16"
applies_to:
  repositories: ["ggml-org/llama.cpp@788e07dc91d266ad3162a1ce9037665656269689"]
  software_versions: []
  hardware_revisions: ["dual Strix Halo"]
related_sections: ["44", "73", "84"]
---

# Procedures and checks

## Internet/source work completed

1. Pin and inspect upstream source at `788e07d`; record `src/llama-graph.cpp`, `src/llama-model.cpp`, and `src/llama-arch.cpp` [S34-01].
2. Compare model contracts against their papers/model metadata; do not infer one model's router from another [S34-03, S34-04, S34-05].
3. Re-check CachyLLama and llama-ai branches for telemetry before implementation [S34-02].

## On-machine experiment M34-01: routing trace

Prerequisites: exact GGUF hash, pinned binary, fixed corpus manifest, no root required.

1. Add an opt-in callback after `ffn_moe_topk` that copies only expert IDs/weights needed for statistics; leave default execution unchanged.
2. Run prompt-fill and decode separately at batch sizes `1, 2, 4, 8` and at least three workload classes.
3. Save environment, model hash, prompt hashes, warmup policy, raw trace checksum, and summary JSON.
4. Repeat each cell; compare per-layer frequency, weighted frequency, entropy, Gini, co-selection, churn, and phase drift.
5. Acceptance: instrumentation-disabled output is bitwise unchanged; enabled output matches within the project's deterministic policy and adds less than a separately declared overhead ceiling.

## On-machine experiment M34-02: placement A/B

Compare whole-layer baseline, static hot-expert replication, and no replication with identical model, requests, sampling seeds, batch schedule, and thermals. Record tokens/s, TTFT, p50/p95 inter-token latency, link bytes, per-rank memory, kernel time, and output equality. A proposal advances only if repeated end-to-end results beat baseline without quality change.

## Safety checks

- Never learn hotness from warmup-all-experts execution.
- Version trace schema and router policy with the model fingerprint.
- If instrumentation fails or IDs are out of range, disable placement optimization and use the baseline graph.
