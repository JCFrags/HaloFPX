---
section_id: "44"
title: "MoE Hybrid Facts and Constraints"
status: "needs-machine-validation"
last_verified: "2026-07-16"
applies_to:
  repositories: ["charlie12345/ROCmFPX@a5605a72768c6562241b248e268e33dc92787394", "ggml-org/llama.cpp@788e07dc91d266ad3162a1ce9037665656269689"]
  software_versions: ["vLLM@9354f222042986addf20709e5274fc26e0d09745", "Megatron-LM@740c16e6b80a753bea26232148d9bb2d7f0c827a", "DeepEP@dd758caf451848bd150e1046af3d0a73e5fff38d"]
  hardware_revisions: ["dual AMD Strix Halo; exact BOM and USB4 fabric measurements unresolved"]
related_sections: ["34", "42", "43", "45", "52"]
---

# Facts and constraints

## Model semantics precede placement

- **[VERIFIED]** A top-k MoE router selects expert functions for each token and combines their outputs using router weights; changing the selected logical experts changes the model computation [S44-05][S44-07].
- **[VERIFIED]** DeepSeekMoE separates shared experts from routed experts. Its shared experts capture common knowledge and participate by architecture definition [S44-06]. **[INFERENCE]** HaloFPX may relocate or byte-identically replicate these weights, but it must not treat a shared expert as a cacheable substitute for a routed expert.
- **[VERIFIED]** Switch Transformer documents fixed expert capacity and token overflow in its training design [S44-07]. **[RECOMMENDATION]** HaloFPX inference must not silently drop or reroute overflow assignments unless the exact target model defines that behavior; backpressure or a correctness-preserving slower path is required.
- **[VERIFIED]** Section 34 found model-specific router transforms, group filtering, top-k selection, normalization, and expert-weighted reduction in pinned llama.cpp. A placement layer must preserve that sequence and numerical contract [S44-10].

## Upstream implementation evidence

- **[VERIFIED]** Megatron Core's `MoEAlltoAllTokenDispatcher` permutes tokens, performs expert-parallel all-to-all, processes local experts, performs inverse all-to-all, and unpermutes results. Its implementation also overlaps shared-expert work with communication in supported configurations [S44-03].
- **[VERIFIED]** DeepEP describes high-throughput and low-latency all-to-all dispatch/combine for inference. At the pinned commit its prerequisites name CUDA, NCCL, NVLink, and RDMA [S44-04]. **[INFERENCE]** This is an algorithmic reference only; it is not evidence that DeepEP runs on HIP, RCCL, Vulkan, or USB4.
- **[VERIFIED]** PyTorch `all_to_all_single` splits an input tensor across a process group and concatenates received pieces, with explicit unequal split sizes available [S44-09]. **[INFERENCE]** Unequal splits fit routed expert traffic, but the API contract alone says nothing about target-fabric latency or GPU-visible transport.
- **[VERIFIED]** vLLM's EPLB records expert loads over a configurable window, rearranges at a configurable interval, and supports redundant physical experts [S44-02]. The same documentation warns that replica weights consume memory that could otherwise serve KV cache.
- **[VERIFIED]** FlexMoE demonstrates a monitor-plan-expand/shrink/migrate pattern and uses slowest-rank load in its scheduling model [S44-08]. Its published results are training-system evidence on other hardware, not HaloFPX inference results.
- **[VERIFIED]** FasterMoE's published project description uses expert shadowing: broadcast a popular expert's parameters instead of receiving all of its inputs when its cost model favors that choice [S44-05]. This motivates comparing weight movement against activation movement; it does not set a HaloFPX threshold.

## Current HaloFPX source gap

- **[VERIFIED]** At `ROCmFPX@a5605a7`, `common/arg.cpp` exposes RPC devices; layer, row, and tensor split modes; and CPU placement of all or leading-layer MoE weights. A full-tree inspection for expert ownership, expert parallelism, all-to-all, replicas, and hot-expert control found no runtime facility implementing the plan in this section [S44-01]. Absence is scoped to that commit and those search/code paths.
- **[INFERENCE]** Expert-parallel HaloFPX therefore requires new graph partitioning, token packing, transport, ownership-map, combine, observability, and failure-handling work; command-line composition of existing split flags is insufficient.

## Traffic models

Let `N` be tokens in the active microbatch, `H` hidden width, `b` transmitted bytes per hidden element, `K` selected routed experts per token, and `r` the number of selected expert assignments owned only by the peer divided by `N*K`.

| Pattern | Approximate payload, excluding headers/alignment | Constraint |
|---|---:|---|
| One coarse stage handoff | `N * H * b` per boundary crossing | predictable; layer owner must hold all required experts |
| Naive remote expert dispatch and return | `2 * r * N * K * H * b` | variable and may duplicate a token for multiple remote experts |
| Destination-coalesced dispatch | implementation-dependent, bounded by selected destinations and metadata | requires exact packing/combine proof |

**[INFERENCE]** For decode (`N` often small), fixed per-message latency and synchronization can dominate payload bandwidth. For prefill or continuous batches, coalescing can improve link utilization but increases temporary buffers and queuing. These are hypotheses until experiment `E44-03` and `E44-05` run.

## Memory and state constraints

For each rank, plan admission must account for:

`M_dense + M_attention + M_KV/state + M_shared + M_owned_cold + M_hot_replicas + M_graph/workspace + M_transport + M_safety <= M_usable`.

- **[RECOMMENDATION]** Compute each term from actual loaded tensor/storage allocation and runtime arenas; do not infer fit from nominal parameter count or average bits per weight.
- **[RECOMMENDATION]** Identify a replica by `(model fingerprint, layer, logical expert, tensor-layout fingerprint, quantization, backend)`. A physical replica must contain byte-identical weights or a separately validated equivalent representation.
- **[RECOMMENDATION]** Persist expert telemetry separately from KV/recurrent state. Placement epochs may change where computation runs; they must not silently change cache compatibility or rank-local state ownership.

## Ownership, failure, and fallback

- **[RECOMMENDATION]** Every active plan must name the origin rank for token state, router executor, physical owners for each logical expert, shared-expert owners, combine owner, map epoch, and checksum.
- **[INFERENCE]** A rank loss makes any peer-only cold expert unavailable. Replicating hot experts alone does not create a correct single-node fallback.
- **[RECOMMENDATION]** On rank/transport failure, abort the affected distributed step. Resume only by replaying from a known-safe boundary under a complete compatible plan, or reload a full single-node model if it fits. Never skip missing experts or accept partial combines.
