---
section_id: "03"
title: "Glossary and Naming Facts and Constraints"
status: "needs-machine-validation"
last_verified: "2026-07-16"
applies_to:
  repositories: ["Custom_Inference_Project", "ROCmFPX", "CachyLlama", "llama-ai", "llama.cpp"]
  software_versions: []
  hardware_revisions: []
related_sections: ["43", "49", "57"]
---

# Facts, controlled terms, and constraints

## Project and upstream names

| Preferred term | Definition | Status |
|---|---|---|
| HaloFPX | This custom dual-Strix-Halo inference project/product; not an upstream repository | **[ASSUMPTION]** working name |
| ROCmFPX | `charlie12345/ROCmFPX`, an experimental llama.cpp-derived family adding AMD-focused GGUF model-weight formats and kernels | **[VERIFIED]** repository description at `a5605a7...` [S03-01] |
| CachyLlama | `fewtarius/CachyLlama`, a llama.cpp fork describing persistent on-disk K/V cache for local agent workloads | **[VERIFIED]** repository description at `6be7459...` [S03-02] |
| llama-ai | `fewtarius/llama-ai`; use the repository name exactly and cite a commit for every behavior claim | **[VERIFIED]** repository identity at `1017f3d...` [S03-03] |
| llama.cpp | `ggml-org/llama.cpp`, upstream C/C++ inference repository | **[VERIFIED]** [S03-04] |
| GGUF | Binary format for models used by GGML executors, designed for extensibility and fast loading | **[VERIFIED]** [S03-05] |

## Topology and execution

| Term | Controlled meaning |
|---|---|
| node | One physical Strix Halo host with its OS, memory, APU, storage, and network interfaces |
| rank | One runtime participant with a stable `rank_id`; rank ownership and restart semantics must be stated |
| coordinator | Rank/process assigning or sequencing distributed work; not assumed to own all model state |
| replication | Each serving replica owns a complete independently executable model state |
| remote speculative decoding | A draft participant proposes tokens and a target participant verifies them; draft and target identity must be recorded |
| tensor parallelism | A single model operation is partitioned across ranks with required synchronization; axis and collective must be specified |
| pipeline parallelism | Ordered model stages reside on different ranks; stage boundaries and microbatch schedule must be specified |
| MoE-aware hybrid | Experts and/or shared layers use different placement/parallel rules; expert ownership and routing must be specified |
| single-node fallback | Declared mode that remains correct after remote rank/fabric removal; degraded capacity is allowed if documented |
| `llamacpp-layer-split` | llama.cpp `--split-mode layer`: layers and K/V are split across devices, described as pipelined [S03-06] |
| `llamacpp-row-split` | llama.cpp `--split-mode row`: weights split by rows and parallelized [S03-06] |
| `llamacpp-tensor-split` | llama.cpp experimental `--split-mode tensor`: weights and K/V split and parallelized at the cited commit [S03-06] |

## Model and cache

| Term | Controlled meaning |
|---|---|
| model artifact | Exact model bytes plus format/metadata; identify with SHA-256 |
| weight quantization | Encoding of model-weight tensors, e.g. a ROCmFPX GGUF quant; not a cache type |
| K/V cache | Runtime attention key/value state associated with evaluated token history |
| rank-local persistent cache | Persistent cache owned and validated by one rank; portability is not implied |
| cache key | Canonical compatibility identity for reusable state; must include every correctness-critical input |
| cache hit | State accepted only after compatibility and integrity validation |
| cache miss | Recompute path; corruption must resolve to miss/recompute, never accepted state |

## Metrics and units

- `tok/s`: tokens per second; qualify `prompt`, `decode`, or `end-to-end`, and state aggregation.
- `ms/token`: milliseconds per generated token; state warm-up and percentile.
- `TTFT_ms`: time to first output token in milliseconds.
- `latency_p50_ms`, `latency_p95_ms`, `latency_p99_ms`: percentile request latency.
- `GB/s` and `Gb/s`: decimal bytes and bits per second; `GiB/s` for binary bytes.
- **[VERIFIED]** SI prefixes are decimal and must not represent powers of 1024; IEC-style binary prefixes distinguish those quantities [S03-07].
