---
title: Qwen3-30B-A3B worked example
status: calculated + explicit routing scenario
---

# Qwen3-30B-A3B-Instruct-2507

## Architecture inputs

**SOURCED FACT.** Official configuration: \(L=48\), residual hidden width \(h=2048\), 32 query heads, 4 KV heads, explicit head dimension \(d=128\), 128 experts, top-8 routing, vocabulary 151,936, and max position embeddings 262,144. [Official Qwen config](https://huggingface.co/Qwen/Qwen3-30B-A3B-Instruct-2507/blob/main/config.json).

The explicit head dimension is used for KV arithmetic even though it differs from \(h/H_q\). Boundary activations use residual width \(h\).

## Memory arithmetic

Ideal Q4 weight lower bound using the marketed 30B total:

\[
30\times10^9\cdot0.5=15\ \text{GB decimal}=13.970\ \text{GiB}.
\]

KV per token:

\[
K_{token}=2\cdot48\cdot4\cdot128\cdot2
=98{,}304\ \text{B}=96\ \text{KiB}.
\]

| Context | Whole-model ideal BF16 KV |
|---:|---:|
| 4,096 | 384 MiB |
| 131,072 | 12 GiB |
| 262,144 | 24 GiB |

## One-cut layer-local expert placement

Boundary per token:

\[
hb_a=2048\cdot2=4096\ \text{B}=4\ \text{KiB}.
\]

| Phase | Payload | One-link nominal floor | Dual-link arithmetic floor |
|---|---:|---:|---:|
| Prefill, 4096 | 16 MiB | 3.355 ms | 1.678 ms |
| Decode, Q=1 | 4 KiB + token feedback | 0.819 µs bulk only | 0.410 µs bulk only |

Experts remain with their layer owner. Because every layer is configured as MoE (`decoder_sparse_step: 1` and no MLP-only layers), cut memory should use actual expert tensor sizes.

## Tensor parallel, TP=2

Forward collectives:

\[
2L=96.
\]

| Phase | Per-rank sent bytes | One-link nominal floor | Dual-link arithmetic floor |
|---|---:|---:|---:|
| Prefill, 4096 | 1.5 GiB | 322.123 ms | 161.061 ms |
| Decode, Q=1 | 384 KiB | 78.643 µs | 39.322 µs |

Add \(96m_{AR}\) latency phases and a distributed sampler or full-logit gather. The vocabulary is 151,936, so a full BF16 logit vector is about 296.75 KiB per sequence per step—large enough to be explicit in TP sampling analysis.

## Remote expert-service sensitivity

**SCENARIO ASSUMPTION.** \(\rho=0.5\). Top-8 routing produces four remote assignments per token per layer on average:

\[
R_l=1\cdot8\cdot0.5=4.
\]

Ignoring metadata:

\[
V_{layer}=4\cdot2\cdot2048\cdot2=32\ \text{KiB}.
\]

Across 48 layers:

\[
V_{dec}=1.5\ \text{MiB/token-step},
\]

\[
V_{pf,4096}=6\ \text{GiB}.
\]

Synchronization: 96 dispatch/return one-way phases at layers with remote assignments. The payload ratio versus one cut is:

\[
2Lk\rho=2\cdot48\cdot8\cdot0.5=384.
\]

This large multiplier is an architecture sensitivity result. Actual \(\rho_l\) can be lower with optimized placement or higher in tails and must be measured.

## Placement implications

- The ideal weight lower bound does not require distribution; actual checkpoint/runtime/KV fit remains the gate.
- For multi-request throughput after fit, replicated decode avoids all model-path USB4 traffic.
- If long context/concurrency forces a split, keep experts local to layer owners.
- Remote expert service faces top-8 fan-out at every layer and is a capacity fallback only after routing optimization.
- TP has 96 collectives and a large-vocabulary sampler cost; fail closed until measured.

## Decision

**Replicated decode:** first throughput candidate after fit. **Layer-local split:** capacity candidate. **Remote expert service:** experimental with trace gate. **TP:** conditional and sampler-sensitive.
