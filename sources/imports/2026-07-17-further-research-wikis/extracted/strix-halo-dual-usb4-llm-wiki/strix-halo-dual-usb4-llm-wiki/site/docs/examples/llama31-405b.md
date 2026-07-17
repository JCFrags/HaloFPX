---
title: Llama 3.1 405B worked example
status: calculated
---

# Llama 3.1 405B

## Architecture inputs

**SOURCED FACT.** Official Meta MP8 SKU fields: \(L=126\), \(h=16384\), \(H_q=128\), \(H_{kv}=8\), \(d=128\), vocabulary 128,256. [Meta `sku_list.py`](https://github.com/meta-llama/llama-models/blob/main/models/sku_list.py).

The official source also contains other model-parallel variants; this example uses the MP8 architecture entry with 8 KV heads. Runtime checkpoint layout must match.

## Weight capacity lower bounds

| Packed weight precision | Whole-model ideal lower bound | Ideal equal half |
|---:|---:|---:|
| BF16/FP16 | 810 GB decimal | 405 GB decimal |
| Q8 | 405 GB decimal | 202.5 GB decimal |
| Q4 | 202.5 GB decimal = 188.593 GiB | 101.25 GB decimal = 94.296 GiB |

**CALCULATED CAPACITY REJECTION.** Even ideal Q8 weights exceed the combined 256 GB maximum platform memory of two 128 GB systems before any KV or runtime allocation, so Q8 cannot fit this two-node topology.

**CONDITIONAL Q4 CAPACITY.** Ideal Q4 weight halves are below 128 GB each, but the margin must absorb quantization overhead, nonuniform layer/head tensors, KV, workspace, OS, and usable-memory limits. The lower bound is not evidence that a specific checkpoint fits.

## KV arithmetic

\[
K_{token}=2\cdot126\cdot8\cdot128\cdot2
=516{,}096\ \text{B}=504\ \text{KiB}.
\]

| Context | Whole-model KV | Equal 63/63 layer split per rank |
|---:|---:|---:|
| 4,096 | 1.969 GiB | 0.984 GiB |
| 16,384 | 7.875 GiB | 3.938 GiB |
| 131,072 | 63 GiB | 31.5 GiB |

These values exclude allocator blocks and metadata. Long context can consume the residual memory left after Q4 weights.

## Contiguous layer split

Boundary per token:

\[
hb_a=16384\cdot2=32768\ \text{B}=32\ \text{KiB}.
\]

| Phase | Payload | One-link nominal floor | Dual-link arithmetic floor |
|---|---:|---:|---:|
| Prefill, 4096 tokens | 128 MiB | 26.844 ms | 13.422 ms |
| Decode, Q=1 | 32 KiB + token feedback | 6.554 µs bulk only | 3.277 µs bulk only |

Synchronization: one boundary per prefill chunk; one boundary plus token return per decode step. A 63/63 cut is arithmetically possible, but actual weight balance must include embeddings/head and quantization tensor sizes.

## Tensor parallel, TP=2

Forward collectives:

\[
2L=252.
\]

| Phase | Per-rank sent bytes | Aggregate bidirectional cut | One-link nominal floor / rank | Dual-link arithmetic floor / rank |
|---|---:|---:|---:|---:|
| Prefill, 4096 tokens | 31.5 GiB | 63 GiB | 6.765 s | 3.382 s |
| Decode, Q=1 | 7.875 MiB | 15.75 MiB | 1.652 ms | 0.826 ms |

Add \(252m_{AR}\) fixed-latency phases and sampler communication. These payload-only floors are already much larger than the one-cut payload floors; this does not prove runtime speed, but it establishes the communication burden TP must overcome with measured compute savings.

Decode speed gate:

\[
252\left(m_{AR}\ell+\frac{32768}{B}\right)
<C_1\left(1-\frac{1}{2\eta_{TP}}\right).
\]

## Placement implications

### Contiguous split

This is the first capacity placement to model because whole layers can be divided and KV remains local. The hard problem is memory margin around ideal Q4, not cross-node volume.

### Pipeline

Can improve aggregate throughput for independent requests if the 63/63 or another feasible cut is compute-balanced and the minimum-concurrency equation passes. It does not reduce one sequence's serial token dependency.

### TP

Use only if a whole-layer Q4 partition cannot fit or the measured TP inequality passes. The 252 collective dependencies make decode latency especially sensitive.

### Replication

Two full Q4 ideal copies would require 405 GB decimal just for packed weights, exceeding the combined platform maximum. Replicated decode is therefore rejected for this topology at Q4 by the ideal weight lower bound.

## Decision

- **Q8 or higher weight storage:** hard NO-GO on two 128 GB systems by ideal lower bound.
- **Q4 contiguous split:** CONDITIONAL on actual checkpoint/runtime memory with context reserve.
- **Pipeline:** CONDITIONAL for concurrent requests after split feasibility.
- **TP:** fail closed; capacity fallback or strict empirical speed gate.
- **Replicated decode:** NO-GO at ideal Q4 weight capacity on this topology.
