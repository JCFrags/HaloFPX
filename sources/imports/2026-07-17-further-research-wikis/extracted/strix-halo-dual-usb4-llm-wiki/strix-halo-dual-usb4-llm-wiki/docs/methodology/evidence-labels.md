---
title: Evidence labels
status: method
---

# Evidence labels

Every material statement and number in this wiki belongs to one of five categories.

## SOURCED FACT

A fact directly supported by a primary source: a vendor product specification, operating-system documentation, official model configuration, official model paper, or original parallelism paper.

Examples:

- two native USB4 40 Gb/s ports on Ryzen AI Max+ 395;
- 32 layers and hidden size 4096 in the official Mixtral configuration;
- two forward all-reduces per transformer layer in Megatron-style tensor parallelism.

A sourced nominal interface rate is not an observed application rate.

## CALCULATED

Arithmetic using sourced fields or declared variables. The calculation must be reproducible from `tools/cost_model.py` or a displayed equation.

Examples:

\[
K_{token}=2LH_{kv}db_{kv}
\]

and, for Llama 3.1 8B at \(b_{kv}=2\) bytes, \(K_{token}=128\) KiB.

A **CALCULATED LOWER BOUND** adds idealizing assumptions such as no protocol overhead or perfectly packed 4-bit weights. It is not a fit or performance claim.

## MEASURED INPUT REQUIRED

A value intentionally left blank because the platform specification cannot determine it. Required measured inputs include:

- effective payload bandwidth by message size and direction;
- one-way fixed message cost;
- simultaneous-link scaling;
- collective phase count and software overhead;
- per-stage compute time and interference;
- model-usable memory;
- speculative acceptance and verifier cost;
- MoE remote assignment fraction and imbalance.

The cost-model CLI accepts these values when available; the repository does not invent defaults.

## SCENARIO ASSUMPTION

A declared value used for sensitivity analysis. It must be visibly labeled and must not be described as likely, typical, or measured without evidence.

Examples in this repository:

- BF16/FP16 activations and KV at two bytes per element;
- \(\rho=0.5\) for an illustrative expert-placement traffic calculation;
- a constant independent speculative acceptance probability \(a\).

## DECISION RULE

An inequality or gate that converts measured inputs into a disposition. For example:

\[
2L\left(m_{AR}\ell+\frac{S}{B}\right)
< C_1\left(1-\frac{1}{2\eta_{TP}}\right)
\]

is the tensor-parallel speed break-even condition under the displayed compute decomposition.

## Prohibited transformations

Do not:

- relabel a nominal USB4 rate as effective payload bandwidth;
- use total 256 GB across two nodes as if it were coherent memory;
- infer tokens/s from parameter count or memory bandwidth alone;
- use model file size as exact runtime residency;
- treat a routing average from another workload as local \(\rho_l\);
- treat a lower-bound transfer time as expected latency;
- claim lossless stochastic speculation without the proposal-distribution and RNG protocol.
