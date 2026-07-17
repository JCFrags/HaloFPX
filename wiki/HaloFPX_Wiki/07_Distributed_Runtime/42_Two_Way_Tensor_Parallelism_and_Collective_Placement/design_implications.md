---
section_id: "42"
title: "Tensor Parallel Design Implications"
status: "needs-machine-validation"
last_verified: "2026-07-16"
applies_to:
  repositories: ["HaloFPX design candidate"]
  software_versions: []
  hardware_revisions: ["dual Strix Halo premise"]
related_sections: ["38", "39", "44", "45", "48", "51", "52"]
---

# Design implications

## Collective plan

For each ordinary block:

1. Both ranks consume the same replicated normalized hidden state.
2. Each computes assigned Q/K/V and local attention.
3. Each computes its output-projection partial `[M,H]`.
4. All-reduce sum; fuse post-reduction bias/residual only if mathematically and numerically validated.
5. Each computes local gate/up, activation, and down-projection partial.
6. All-reduce sum; apply validated bias/residual and continue.

**[RECOMMENDATION]** Give every collective an immutable `(cluster_epoch, session epoch, iteration, layer, site, shape, dtype, plan)` identity. Ranks must enter in identical order; mismatches time out/fault rather than consume the wrong buffer.

## Two-link placement and buffers

**[RECOMMENDATION]** Preallocate double-buffered, aligned send/receive/accumulation storage per size bucket. Initially implement a correct single-link two-rank sum, then compare:

- striping disjoint halves over two independent links;
- one reduce direction per link followed by exchange;
- size-based single-link versus dual-link selection.

Reassembly must validate range, round, and checksum/transport integrity. Do not assume the two paths are independent or ordered. Buffer reuse waits on both compute and transport completion events.

## Fusion/overlap opportunities

- Fuse dequantization/GEMM epilogues into local partial production where kernels support it.
- Fuse bias/residual/norm after reduction only when bias is applied exactly once.
- Chunk a large reduction and overlap completed chunks with local work only where graph dependencies permit.
- Batch/aggregate small collective operations when semantically independent; RCCL docs note aggregation can help small operations [S42-03].
- Do not claim overlap if the next operation requires the complete replicated hidden state.

## Prefill versus decode

Prefill has larger `M`, larger GEMMs, and larger reductions; it may amortize launch latency but stresses payload bandwidth. Decode has `M` equal to active sequences (often one), very small GEMMs, and two latency-sensitive reductions per layer; fixed transport/software jitter can dominate. Measure and select independently; a system may use TP for capacity even when it is slower.

## Output head choices

| Choice | Memory | Traffic/semantics |
|---|---|---|
| rank-0 head | full head on rank 0 | no vocab gather; rank 0 compute bottleneck |
| replicated head | duplicate head | no collective; duplicate compute optional |
| vocab-sharded head + gather | half head each | large `V` logit gather to sampler |
| vocab-sharded distributed sampler | half head each | lower possible traffic; complex exact top-k/top-p/penalty/grammar semantics |

**[RECOMMENDATION]** Start with rank-0 or replicated head for correctness, then optimize only if memory/compute requires it.

## Break-even

For a block and workload bucket, require measured `p99(max(C0_shard,C1_shard) + R_attn + R_ffn + sync/jitter) < p99(C_single_block)` or a capacity necessity. Test end-to-end because per-layer p99 values cannot simply be summed. Include graph launch, copies, queueing, cache, head, and failure penalty from section 38.

## Architecture variants

**[RECOMMENDATION]** Generate a per-model tensor map from exact metadata/source. Reject unsupported shapes rather than guessing. Parallel attention+FFN blocks may allow a different reduction fusion; MoE uses section 44; tied embedding/head affects placement; MLA/MQA/GQA, sliding window, recurrent state, multimodal projectors, and native MTP each require dedicated ownership rules.
