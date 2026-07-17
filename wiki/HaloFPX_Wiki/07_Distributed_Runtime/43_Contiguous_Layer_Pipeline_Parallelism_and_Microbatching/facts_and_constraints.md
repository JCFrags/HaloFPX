---
section_id: "43"
title: "Pipeline Parallel Facts and Constraints"
status: "needs-machine-validation"
last_verified: "2026-07-16"
applies_to:
  repositories: ["charlie12345/ROCmFPX@a5605a72768c6562241b248e268e33dc92787394", "ggml-org/llama.cpp@788e07dc91d266ad3162a1ce9037665656269689", "vllm-project/vllm@9354f222042986addf20709e5274fc26e0d09745"]
  software_versions: ["PyTorch distributed.pipelining 2.13 documentation"]
  hardware_revisions: []
related_sections: ["32", "38", "39", "45", "46", "48", "51", "58"]
---

# Pipeline parallel facts and constraints

## Current implementation evidence

- **[VERIFIED]** ROCmFPX commit `a5605a7` documents `--split-mode layer` as the default multi-device mode and describes it as splitting layers and KV across devices in a pipelined layout. Its loader computes cumulative split points from free memory or `--tensor-split`, then assigns successive layer indices and the output layer by those ranges [S43-01].
- **[VERIFIED]** At that commit, ROCmFPX enables its ggml pipeline scheduler only for multiple devices with layer split, full relevant offload, K/Q/V offload, no tensor-buffer overrides, and async-compute/event support on every accelerator. It can retry allocation without pipeline scheduling, and graph reuse synchronizes before overwriting inputs [S43-01]. This establishes current scheduler preconditions, not HaloFPX cross-host microbatch semantics.
- **[VERIFIED]** ROCmFPX's RPC documentation calls the backend proof-of-concept, fragile, and insecure. It exposes remote devices and says weights and KV are distributed across local and remote devices by available memory unless overridden [S43-02]. This is not evidence that the intended dual-link transport, failure recovery, or safe production protocol exists.
- **[VERIFIED]** Upstream llama.cpp commit `788e07d` has the same documented layer-split concept and conditions pipeline scheduling on async/event-capable devices. The code explicitly notes that pipeline scheduling increases memory use [S43-03].
- **[VERIFIED]** vLLM commit `9354f22` computes a contiguous `[start_layer,end_layer)` interval per pipeline rank, permits an explicit `VLLM_PP_LAYER_PARTITION`, and adjusts the default split because the first/last partitions may also own embeddings, norm, or output work. Unsupported models fail unless they implement its pipeline interface [S43-04].
- **[VERIFIED]** In that vLLM revision, a non-final pipeline stage returns intermediate tensors, while the final stage computes logits. vLLM documents PP as an inference/serving capacity mechanism across nodes or for uneven layer splits [S43-04]. Its software and hardware stack differ from HaloFPX.
- **[VERIFIED]** PyTorch's versioned pipeline API separates models into stages, splits an input batch into microbatches, and offers a fill-drain GPipe schedule; it warns that stage input/output shapes must remain static and that shape mismatch raises an error [S43-05].
- **[VERIFIED]** Orca's OSDI 2022 system schedules autoregressive serving at iteration granularity so requests may enter or leave between iterations [S43-07]. Sarathi identifies unequal prefill/decode work as a source of pipeline bubbles and uses chunked prefills plus decode-maximal batches to make work units more uniform [S43-06]. Their published measurements do not transfer to Strix Halo or USB4.

## Two-stage forward-only cost model

Let `M` be tokens in a pipeline work unit, `H` hidden width, `e` bytes per transferred activation element, `C0(M)` and `C1(M)` stage compute times, and `X(B)` the measured end-to-end boundary-transfer time. A simple hidden tensor has logical payload `B = M * H * e`; models that also cross residuals, masks, routing metadata, or multimodal state have a larger manifest-defined bundle.

**[INFERENCE]** For `m` ready work units with fixed order and no cross-unit dependency, a two-stage makespan is approximately:

`C0 + X + C1 + (m - 1) * max(C0, X, C1)`

when compute and link resources can overlap perfectly, before host scheduling, resource contention, and drain overhead. If both stage service times are `t` and transfer is hidden within them, fill-drain takes `(m+1)t`, so ideal per-stage utilization is `m/(m+1)`. This arithmetic is an optimistic model, not a measurement [S43-05, S43-08].

## Autoregressive and ownership constraints

- **[INFERENCE]** One sequence supplies at most one ordinary decode work unit at a time: the next input token is unavailable until the suffix stage produces logits and sampling commits the prior token. Independent sequences are therefore the primary decode microbatches that can overlap stages.
- **[INFERENCE]** KV need not cross the layer boundary during steady-state execution. Rank 0 owns KV for its prefix layers and rank 1 owns KV for its suffix layers. Both halves must share session, epoch, token-position, cache-layout, and model fingerprints.
- **[INFERENCE]** A contiguous layer split sends boundary activations once per work unit, unlike layer-wise tensor parallelism's repeated collectives. The comparison must still include copies and tail latency; theoretical link rate is insufficient.
- **[ASSUMPTION]** The two Strix Halo hosts can expose a sufficiently reliable ordered data path for the boundary bundle. Dual-link independence, direct GPU visibility, effective bandwidth, and p99 latency are unverified.
- **[ASSUMPTION]** Target architectures can be cut between complete decoder blocks. Hybrid/recurrent state, multimodal branches, tied embedding/head weights, MoE routing, and native MTP may require extra ownership rules or exclusion.

## Capacity and failure constraints

- **[INFERENCE]** Each rank stores only its assigned weights and layer-local KV in a true rank-local implementation, but embeddings/head, transfer buffers, graph copies, and allocator headroom make memory unequal. Layer count alone is not a safe partition metric.
- **[INFERENCE]** Losing either required stage invalidates an in-flight same-model pipeline request. If the model does not fit one node, a same-model single-node fallback is impossible; recovery requires both ranks or an explicitly smaller model.
- **[RECOMMENDATION]** Treat an incomplete, duplicate, stale, corrupt, wrong-shape, or wrong-epoch boundary bundle as a failed work unit. Never pass it to the suffix stage or accept partial KV as a cache hit.
