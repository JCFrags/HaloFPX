---
section_id: "43"
title: "Contiguous Layer Pipeline Parallelism and Microbatching"
status: "needs-machine-validation"
last_verified: "2026-07-17"
applies_to:
  repositories: ["charlie12345/ROCmFPX@a5605a72768c6562241b248e268e33dc92787394", "ggml-org/llama.cpp@788e07dc91d266ad3162a1ce9037665656269689", "vllm-project/vllm@9354f222042986addf20709e5274fc26e0d09745"]
  software_versions: ["PyTorch distributed.pipelining 2.13 documentation"]
  hardware_revisions: ["two matched AMD Strix Halo systems; exact revisions open"]
related_sections: ["32", "38", "39", "42", "44", "45", "46", "48", "51", "52", "58"]
---

# 43 - Contiguous Layer Pipeline Parallelism and Microbatching

**[RECOMMENDATION]** Define pipeline mode as a two-rank, forward-only pipeline: rank 0 owns the embedding and a contiguous prefix of transformer layers; rank 1 owns the remaining contiguous suffix, final norm, output head, and the physical sampling fast path. Transfer only a versioned boundary-tensor bundle in the forward direction and a committed token/result envelope in the reverse direction.

**[INFERENCE]** Pipeline mode primarily expands model and KV capacity. A single autoregressive sequence cannot keep both stages busy continuously because token `t+1` depends on the sampled result of token `t`. Throughput overlap requires multiple independent ready sequences or prefill chunks, and its benefit must exceed stage imbalance, boundary transfer, queueing, and fill/drain bubbles [S43-05, S43-06, S43-07].

**[OPEN]** No HaloFPX two-node pipeline, activation-transfer, long-prefill, failure-recovery, or break-even measurement exists. Current ROCmFPX layer mode is relevant implementation evidence, not proof of the proposed rank protocol or performance over the intended USB4 fabric.

**[OPEN]** Rank-1 physical sampling is a candidate placement, not a selected contract. Sampler/RNG/grammar state transfer and validation, boundary tensor transport, atomic two-rank KV checkpointing, recurrent/MTP/MoE state ownership, and safe fallback remain unresolved in DR43-O3, DR43-O4, DR43-O7 through DR43-O10. Do not implement transparent continuation from the proposed flow until those tests close.

## Authoritative pages

- [Facts and constraints](facts_and_constraints.md) records commit-pinned implementation facts and the cost model.
- [Design implications](design_implications.md) defines the provisional ownership, partition, scheduling, and recovery design.
- [Procedures and checks](procedures_and_checks.md) separates completed source research, required machine experiments, and contingent decisions.
- [Open questions](open_questions.md) is the section decision backlog.
- [Sources](sources.md) records provenance and applicability limits.

## Scope boundary

Section 39 owns coordinator/rank/session authority; section 45 owns the persistent command protocol; section 46 owns global admission and continuous batching; section 48 owns correctness and degraded mode; sections 51-54 own transport behavior; section 58 owns distributed cache restore. This section owns layer boundaries, boundary tensors, pipeline work units, and their scheduling constraints.

## Improvement review

- Correctness: upstream behavior is separated from HaloFPX recommendations; no benchmark is promoted.
- Freshness: moving repositories are pinned to heads observed on 2026-07-16 PT.
- Main gaps: real per-layer times, rank memory ceilings, boundary-copy tails, graph behavior, and offered-load traces.
- Review trigger: any pinned runtime/model-layout change or completion of experiment `DR-43-E1` through `DR-43-E5`.
