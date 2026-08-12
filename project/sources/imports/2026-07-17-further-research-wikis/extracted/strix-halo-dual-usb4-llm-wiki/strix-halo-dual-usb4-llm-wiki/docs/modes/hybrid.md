---
title: Hybrid approaches
status: conditional composition
---

# Hybrid approaches

A hybrid is viable only when every component's ownership, memory, correctness, and break-even gates pass together. Component benefits are not additive when they contend for the same memory bandwidth, runtime queues, or USB4 paths.

## Hybrid 1: contiguous placement + pipeline schedule

This is the lowest-risk hybrid.

- **Ownership:** unchanged from the contiguous layer split.
- **Execution:** independent prompts/sequences are microbatched through the two stages.
- **Prefill/decode volume:** unchanged in total; more messages due to microbatching.
- **KV:** local by layer.
- **Sampler:** rank 1.
- **Use:** model capacity plus aggregate throughput.

Machine-readable definition: [`placements/hybrid-layer-pipeline.yaml`](../../placements/hybrid-layer-pipeline.yaml).

GO requires both the contiguous memory gates and pipeline service/concurrency gates.

## Hybrid 2: MoE layer-local split + pipeline

Keep all experts with their layers, then pipeline independent work across the single boundary.

- **Expert traffic:** none beyond the one hidden-state cut.
- **Cut selection:** account for expert-weight and compute imbalance.
- **Synchronization:** one boundary per microbatch, not two phases per MoE layer.

This is the preferred hybrid for Mixtral- or Qwen-style models when a layer-local partition fits.

## Hybrid 3: split target + co-resident drafter

Rank 0 can host the first target stage and a small drafter; rank 1 hosts the second target stage and authoritative target sampler.

Possible round:

1. rank 0 drafts candidate IDs using draft KV;
2. target verification itself executes through rank 0's first target layers, one hidden-state boundary, and rank 1's later target layers;
3. rank 1 commits/corrects and returns token IDs.

Ownership:

- **Tokenizer/session/draft/draft KV:** rank 0;
- **target model/KV:** split by layers;
- **target sampler/final RNG:** rank 1.

The target verification communication is the contiguous boundary for the entire candidate block, plus speculative control. This can be attractive for target capacity, but rank 0's drafter competes with target stage A for unified memory bandwidth and workspace.

Break-even:

\[
T_d(\gamma)+T_{target\_split\_verify}(\gamma)+T_{control}
<E[K]T_{target\_split,1}.
\]

All terms must be measured under co-residency; adding separately measured isolated times is insufficient.

## Hybrid 4: remote expert service inside a layer split

Some layers live on each rank, while selected experts for a local layer live remotely. This creates both:

- the one main layer boundary; and
- dispatch/return traffic at every layer whose experts cross the cut.

For prefill:

\[
V=Nhb_a+\sum_l N_lk\rho_l(2hb_a+b_{meta}).
\]

For decode:

\[
V=Qhb_a+Qb_t+b_{control}+\sum_l Qk\rho_l(2hb_a+b_{meta}).
\]

This is justified only when layer-local expert placement cannot satisfy memory. Expert placement should be optimized within each side before adding cross-cut experts.

## Hybrid 5: replicated small models, split large models

A frontend selects an execution class by model and context:

- models fitting one node use independent replicas;
- larger models use a contiguous split;
- very large/per-layer constrained models use a gated TP or expert-service placement;
- remote speculation is enabled only for validated drafter/target pairs.

This portfolio hybrid does not combine modes within one request. It uses dual USB4 efficiently by keeping model-path communication off the link for small models and reserving it for capacity placements.

Ownership is mode-specific and fixed at admission. Do not migrate a session between execution classes without an explicit KV/re-prefill protocol.

## Hybrid 6: dual-link control/bulk separation

Use one link for low-latency ordered control and the other, or both after validation, for bulk tensors.

- control: token feedback, session state, cancellation, heartbeat;
- bulk: hidden activations, collective chunks, expert dispatch, probability vectors, KV migration.

This is a transport hybrid, not automatic bandwidth aggregation. It can reduce head-of-line blocking even when striping does not improve one-message throughput.

## Hybrid 7: prefill/decode disaggregation with KV migration

Rank 0 performs prefill and rank 1 performs decode after KV transfer.

Minimum transfer:

\[
V_{KV}=2cLH_{kv}db_{kv}.
\]

Break-even:

\[
T_{export}+\ell+\frac{V_{KV}}{B}+T_{import}
<G\Delta T_{decode}.
\]

This layout additionally requires both nodes to hold an execution-capable copy/placement for their phase. No phase-specific Strix Halo advantage is assumed. The default disposition is **NO-GO without new evidence**.

Machine-readable definition: [`placements/hybrid-prefill-decode-migration.yaml`](../../placements/hybrid-prefill-decode-migration.yaml).

## Hybrid 8: TP prefill, different decode placement

Changing from TP prefill to replicated or layer-split decode requires transforming KV ownership and possibly weight placement. At minimum, all prompt KV must be gathered/repartitioned:

\[
V_{transition}\ge cK_{token}
\]

plus layout metadata and model-state transition. Runtime support for this transformation is not assumed. Default: **NO-GO** unless a backend exposes a validated zero/low-copy transition and the amortization gate passes.

## Composition checklist

A hybrid is rejected when any one of these is missing:

- one authoritative tokenizer/token-ID mapping;
- one authoritative final sampler and RNG stream;
- complete model/expert ownership with no duplicate or missing weights;
- exact KV owner before, during, and after transitions;
- combined memory budget including all co-resident models and buffers;
- combined communication/synchronization equation;
- contention trace for shared memory/compute/network resources;
- failure state machine across component boundaries.

## Disposition

The practical hybrids are **layer split + pipeline**, **MoE layer-local split + pipeline**, and **mode-by-model request routing**. Co-resident speculation, cross-cut experts, and phase migration remain conditional or experimental.
