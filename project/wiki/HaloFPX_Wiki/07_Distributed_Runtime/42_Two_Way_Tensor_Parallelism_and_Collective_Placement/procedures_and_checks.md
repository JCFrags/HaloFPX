---
section_id: "42"
title: "Tensor Parallel Procedures and Checks"
status: "needs-machine-validation"
last_verified: "2026-07-16"
applies_to:
  repositories: ["HaloFPX design candidate"]
  software_versions: []
  hardware_revisions: []
related_sections: ["30", "31", "48", "51", "52", "66"]
---

# Procedures and checks

## Source research completed

Pinned Megatron-LM code/paper, RCCL docs/source head, and llama.cpp RPC behavior. The mapping is a design derived from these sources; it is not present/proven in HaloFPX.

## `DR-42-E1`: model shard manifest

For every target model/hash, export tensor names, logical roles, shapes, packed type/block size, head/Q-group/KV-head counts, tied tensors, kernel tile constraints, and proposed axis/range. Independently reconstruct the original tensor from the two shards and hash or compare dequantized values. Reject ambiguous names/roles and non-divisible unsupported layouts.

## `DR-42-E2`: collective microbench

Prerequisites: both nodes, exact kernel/driver/transport revisions. Sweep payloads matching `M*H*e`, from decode batch 1 through maximum prefill/concurrency. Test each link, both links, aligned/misaligned buffers, FP32/FP16/BF16 as supported, warm/cold, and concurrent compute/copies. Retain p50/p95/p99, bytes, CPU/GPU utilization, retries, errors, and result correctness. Compare custom transport and RCCL only where each is actually supported.

## `DR-42-E3`: layer and end-to-end oracle

1. Use deterministic tiny tensors to validate column/row shards and sums against a full reference.
2. Compare per-site pre/post-collective tensors for single-rank versus TP using defined absolute/relative error and top-logit margins.
3. Cover dense MHA, GQA divisible, MQA/odd KV fallback, fused QKV/gated FFN, tied head, each quant type, context shift, cache hit/miss, prefill and decode.
4. Run full token/logit correctness and stochastic distribution tests under section 48.
5. Inject rank delay/loss, collective mismatch, duplicate/stale message, one-link loss, corrupt chunk, and cancellation.

## `DR-42-E4`: break-even matrix

Sweep prompt/output, batch/concurrency, model/quantization, head placement, collective type, graph/eager, link plan, and KV mode. Compare single-node and replication. Report capacity separately from speed; promote only matched p99 results with raw evidence.

## Required acceptance conditions

- All collectives complete in identical order with bounded timeout.
- Unsupported architecture/quant layouts fail closed.
- Corrupt/missing data aborts and recomputes/retries under policy; never accepted.
- No `[MEASURED]` label without environment metadata and raw artifacts.
