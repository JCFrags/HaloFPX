---
title: Mixtral 8x7B worked example
status: calculated + explicit routing scenario
---

# Mixtral 8x7B

## Architecture inputs

**SOURCED FACT.** Official configuration: \(L=32\), \(h=4096\), \(H_q=32\), \(H_{kv}=8\), \(d=128\), 8 experts, top-2 routing, vocabulary 32,000, context 32,768. [Official config](https://huggingface.co/mistralai/Mixtral-8x7B-Instruct-v0.1/blob/main/config.json).

The primary paper reports approximately 47B total and 13B active parameters. [Mixtral of Experts](https://arxiv.org/abs/2401.04088).

## Memory arithmetic

Ideal Q4 total weight lower bound from 46.7B rounded parameters:

\[
46.7\times10^9\cdot0.5=23.35\ \text{GB decimal}=21.746\ \text{GiB}.
\]

This excludes quantization overhead and is not actual residency.

KV is the same arithmetic as Llama 3.1 8B:

\[
K_{token}=128\ \text{KiB}.
\]

At the native 32,768-token context, one sequence's ideal BF16 KV is 4 GiB before allocator/metadata.

## One-cut layer-local expert placement

Boundary per token: 8 KiB.

| Phase | Payload | One-link nominal floor | Dual-link arithmetic floor |
|---|---:|---:|---:|
| Prefill, 4096 tokens | 32 MiB | 6.711 ms | 3.355 ms |
| Decode, Q=1 | 8 KiB + token feedback | 1.638 µs bulk only | 0.819 µs bulk only |

All eight experts attached to a layer stay on that layer's owner. The layer cut must account for expert weights, but network cost remains one boundary.

## Tensor parallel, TP=2

There are 64 forward collectives. Hidden width/layer count match the Llama 8B volume calculation:

| Phase | Per-rank sent bytes | One-link nominal floor | Dual-link arithmetic floor |
|---|---:|---:|---:|
| Prefill, 4096 | 2 GiB | 429.497 ms | 214.748 ms |
| Decode, Q=1 | 512 KiB | 104.858 µs | 52.429 µs |

A true tensor-expert hybrid adds both TP collectives and expert-parallel communication; this example does not hide that cost.

## Remote expert-service sensitivity

**SCENARIO ASSUMPTION.** Set \(\rho=0.5\), meaning half of top-2 assignments are remote. This yields one remote assignment per token per layer on average:

\[
R_l=1\cdot2\cdot0.5=1
\]

for \(Q=1\) decode.

Ignoring metadata:

\[
V_{layer}=1\cdot2\cdot4096\cdot2=16\ \text{KiB}.
\]

Across 32 MoE layers:

\[
V_{dec}=512\ \text{KiB/token-step},
\]

\[
V_{pf,4096}=2\ \text{GiB}.
\]

Synchronization: dispatch and return at every active MoE layer—64 one-way phases before lower-level transport details. Metadata and expert-capacity padding add bytes.

The \(\rho=0.5\) expert-service payload is 64 times the one-cut payload for the same tokens:

\[
\frac{2Lk\rho hb_a}{hb_a}=2\cdot32\cdot2\cdot0.5=64.
\]

This is a sensitivity result, not a routing prediction.

## Placement implications

- **Model fits one node after real residency test:** replicated decode is the first throughput candidate; remote experts have no capacity justification.
- **Needs two nodes:** divide by whole layers and keep all experts layer-local.
- **Expert service:** only when expert capacity prevents a whole-layer split and a workload trace yields low enough \(\rho_l\)/imbalance.
- **TP or tensor-expert hybrid:** strict collective and all-to-all gates; not a default USB4 placement.

## Decision

**Layer-local expert split: GO candidate for capacity.** **Remote expert service: experimental.** Replace \(\rho=0.5\) with traces before any decision. Replicated decode remains preferred when actual full-model and KV residency fit each node.
