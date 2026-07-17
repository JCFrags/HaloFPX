---
title: Llama 3.1 8B worked example
status: calculated
---

# Llama 3.1 8B

## Architecture inputs

**SOURCED FACT.** Official Meta SKU fields: \(L=32\), \(h=4096\), \(H_q=32\), \(H_{kv}=8\), \(d=128\), vocabulary 128,256, and 128K context family. [Meta `sku_list.py`](https://github.com/meta-llama/llama-models/blob/main/models/sku_list.py), [Llama 3.1 model card](https://huggingface.co/meta-llama/Llama-3.1-8B-Instruct).

## Memory arithmetic

At \(b_{kv}=2\):

\[
K_{token}=2\cdot32\cdot8\cdot128\cdot2=131{,}072\ \text{B}=128\ \text{KiB}.
\]

Calculated KV examples for one sequence, excluding allocator/metadata:

| Context | Whole-model KV |
|---:|---:|
| 4,096 | 512 MiB |
| 32,768 | 4 GiB |
| 131,072 | 16 GiB |

Ideal packed weight lower bounds:

| Weight precision | Lower bound |
|---:|---:|
| BF16/FP16 | 16.0 GB decimal |
| Q8 | 8.0 GB decimal |
| Q4 | 4.0 GB decimal = 3.725 GiB |

These exclude all runtime and quantization overhead. Parameter storage alone does not force a two-node placement; long-context/concurrency and actual runtime residency still require measurement.

## Contiguous layer split

Boundary per token:

\[
hb_a=4096\cdot2=8192\ \text{B}=8\ \text{KiB}.
\]

| Phase | Cross-node payload | One-link nominal floor | Dual-link arithmetic floor |
|---|---:|---:|---:|
| Prefill, \(N=4096\) | 32 MiB A→B | 6.711 ms | 3.355 ms |
| Decode, \(Q=1\) | 8 KiB A→B + token feedback | 1.638 µs bulk only | 0.819 µs bulk only |

All floors exclude fixed latency and overhead. Decode still has one activation message and one token-feedback message per token.

An even 16/16 layer split stores 64 KiB of KV per token on each rank, before runtime metadata.

## Tensor parallel, TP=2

Forward collectives:

\[
2L=64.
\]

| Phase | Per-rank sent bytes | Aggregate bidirectional cut bytes | One-link nominal payload floor / rank | Dual-link arithmetic floor / rank |
|---|---:|---:|---:|---:|
| Prefill, \(N=4096\) | 2 GiB | 4 GiB | 429.497 ms | 214.748 ms |
| Decode, \(Q=1\) | 512 KiB | 1 MiB | 104.858 µs | 52.429 µs |

Add \(64m_{AR}\) fixed-latency phases and the sampler protocol. At \(m_{AR}=1\), decode has 64 collective fixed costs; at \(m_{AR}=2\), 128. The actual value is a traced input.

TP speed requires:

\[
64\left(m_{AR}\ell+\frac{8192}{B}\right)
<C_1\left(1-\frac{1}{2\eta_{TP}}\right)
\]

for \(Q=1\) decode. No values for \(C_1,\eta_{TP},B,\ell\) are assumed.

## Replicated decode

A full copy resides on each node, and each session's KV stays on its assigned replica. Cross-node model-path bytes are zero. Given the low ideal weight lower bounds relative to the platform maximum, this is the first throughput placement to test after actual fit is confirmed.

## Remote speculation

Llama 3.1 8B can serve as either a target for a smaller compatible drafter or a drafter for a larger target, depending on tokenizer/model compatibility. The network formula is independent of its hidden width when only candidate token IDs are sent. Viability depends on measured draft/verify times and acceptance, not the 8B label.

## Decision

- **Multi-request throughput:** replicated decode is the default candidate after fit.
- **Capacity split:** normally unnecessary from weight lower bounds alone; may be relevant for extreme KV concurrency.
- **Single-request pipeline:** no-go as a speed argument.
- **TP:** no capacity justification from parameter storage alone and a strict 64-collective gate.
- **Remote speculation:** conditional on a measured compatible drafter/target pair.
