---
section_id: "42"
title: "Tensor Parallel Facts and Mapping"
status: "needs-machine-validation"
last_verified: "2026-07-16"
applies_to:
  repositories: ["NVIDIA/Megatron-LM@740c16e6b80a753bea26232148d9bb2d7f0c827a", "ROCm/rccl@57e58688f44c77076ad536ef1f6b68741fc6e694"]
  software_versions: ["RCCL docs 2.30.4"]
  hardware_revisions: []
related_sections: ["30", "31", "44", "51"]
---

# Facts and constraints

- **[VERIFIED]** Megatron-LM defines column-parallel linear layers by splitting a weight matrix along its output dimension and row-parallel layers along its input dimension; its transformer strategy partitions attention heads and the MLP expansion while placing reductions after row-parallel projections [S42-01, S42-02].
- **[VERIFIED]** RCCL 2.30.4 documents all-reduce, all-gather, reduce-scatter, broadcast, all-to-all and point-to-point collectives for AMD GPUs, with ring/tree algorithms and topology awareness [S42-03]. This does not prove performance or support over HaloFPX's intended USB4 host-to-host path.
- **[VERIFIED]** Current `llama.cpp` RPC says weights/KV are distributed by available memory and `--tensor-split` controls proportions; the same document calls the backend proof-of-concept [S42-04]. Placement/offload must not be mislabeled as this design's intra-layer TP.

## Baseline two-rank transformer mapping

Let `M` be tokens in the current microbatch, `H` hidden width, `I` FFN intermediate width, `V` vocabulary, and `e` activation/logit bytes. `H`, relevant head counts, `I`, and quant block dimensions must satisfy exact shard divisibility or use an explicitly padded/replicated path.

| Tensor/operator | Rank placement | Input/output state | Steady-state collective |
|---|---|---|---|
| normalization/residual | replicated | `[M,H] -> [M,H]` | none |
| Q projection | output-column/head shard | replicated X -> local Q heads | none |
| K/V projection | KV-head shard if divisible; otherwise replicate K/V or reject | local KV heads/cache | none in attention |
| attention | local assigned Q/KV groups | local head output | none |
| output projection | input-row shard | local heads -> partial `[M,H]` | all-reduce sum `[M,H]` |
| FFN gate/up | output-column shard | replicated X -> local `[M,I/2]` each | none |
| activation/gating | local | local intermediate | none |
| FFN down | input-row shard | local intermediate -> partial `[M,H]` | all-reduce sum `[M,H]` |
| embedding lookup | replicate weights, or vocab shard with masked local lookup | token IDs -> `[M,H]` | none if replicated; all-reduce if vocab-sharded |
| output head | replicate, or vocab-column shard | `[M,H]` -> logits | none if rank-0/replicated; all-gather or distributed sampling if sharded |

**[INFERENCE]** A standard dense decoder block therefore needs two hidden-activation reductions, assuming replicated residual/norm state and no sequence parallelism. Architecture-specific fused, parallel-attention/FFN, MoE, recurrent, multimodal, or cross-attention blocks require separate maps.

## Payloads

Each baseline all-reduce has logical tensor bytes `N = M * H * e`. A conventional two-rank ring reduce-scatter plus all-gather moves approximately `N` bytes per rank in aggregate (two half-buffer phases), plus protocol/copy overhead; measure actual bytes and tail latency. Two reductions give approximately `2N` bytes per rank per block, excluding embeddings/head and retransmission.

Vocab all-gather exposes `M * V * e_logits` total logits at each recipient and can dominate decode traffic for large `V`. Exact distributed softmax/sampling can reduce traffic but has sampler-dependent algorithms and must preserve full sampling semantics.

## KV ownership

**[RECOMMENDATION]** Keep KV with its assigned attention heads and never all-gather KV each token. For GQA, `n_kv_heads` must divide two for symmetric sharding. For MQA (`n_kv_heads=1`) or odd counts, duplicate K/V projection/cache or reject symmetric TP; document the memory/compute cost. KV quantization/type/layout identity is rank-local but globally validated.

## Quantization constraints

Quantized weights remain in their owning rank's memory; steady-state collectives carry activations/partials, not weight blocks. Split boundaries must respect packed quantization blocks, scale/zero-point grouping, fused kernel tile requirements, and tensor semantics. Partial sums require an accumulation/collective type validated for numerical error; do not assume quantized activation reduction is lossless.
