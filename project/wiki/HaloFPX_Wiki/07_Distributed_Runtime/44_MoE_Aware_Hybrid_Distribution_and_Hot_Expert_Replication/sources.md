---
section_id: "44"
title: "MoE Hybrid Sources"
status: "needs-machine-validation"
last_verified: "2026-07-16"
applies_to:
  repositories: ["charlie12345/ROCmFPX@a5605a72768c6562241b248e268e33dc92787394", "ggml-org/llama.cpp@788e07dc91d266ad3162a1ce9037665656269689"]
  software_versions: ["vLLM@9354f222042986addf20709e5274fc26e0d09745", "Megatron-LM@740c16e6b80a753bea26232148d9bb2d7f0c827a", "DeepEP@dd758caf451848bd150e1046af3d0a73e5fff38d"]
  hardware_revisions: ["dual AMD Strix Halo; exact BOM and USB4 fabric measurements unresolved"]
related_sections: ["34", "42", "43", "45", "52"]
---

# Sources

Access date for all Internet sources: **2026-07-16**. Source-code links are commit-pinned. Papers and upstream benchmarks describe their own systems only; no reported number is a HaloFPX measurement.

| ID | Primary source and revision | Claims supported | Limitations/conflicts |
|---|---|---|---|
| S44-01 | [ROCmFPX `a5605a72768c6562241b248e268e33dc92787394`, `common/arg.cpp`](https://github.com/charlie12345/ROCmFPX/blob/a5605a72768c6562241b248e268e33dc92787394/common/arg.cpp), commit timestamp 2026-07-16; [llama.cpp `788e07dc91d266ad3162a1ce9037665656269689`](https://github.com/ggml-org/llama.cpp/tree/788e07dc91d266ad3162a1ce9037665656269689), commit timestamp 2026-07-17 +02:00 | current split/RPC/MoE-offload controls; no inspected EP/replica controller | absence conclusion is bounded to pinned trees and searched facilities; current commits are moving targets |
| S44-02 | [vLLM expert-parallel deployment and EPLB code](https://github.com/vllm-project/vllm/blob/9354f222042986addf20709e5274fc26e0d09745/docs/serving/expert_parallel_deployment.md), [`eplb_state.py`](https://github.com/vllm-project/vllm/blob/9354f222042986addf20709e5274fc26e0d09745/vllm/distributed/eplb/eplb_state.py), commit `9354f222...`, timestamp 2026-07-17 +08:00 | load window, rearrangement interval, logical/physical/redundant experts, replica memory warning | vLLM architecture/backends differ from llama.cpp-derived HaloFPX; examples are not AMD/USB4 proof |
| S44-03 | [Megatron-LM `token_dispatcher.py`](https://github.com/NVIDIA/Megatron-LM/blob/740c16e6b80a753bea26232148d9bb2d7f0c827a/megatron/core/transformer/moe/token_dispatcher.py), commit `740c16e6...`, 2026-07-16 | token permutation, EP all-to-all, inverse combine, shared-expert overlap pattern | CUDA/PyTorch/NCCL-oriented training and inference code; not directly portable |
| S44-04 | [DeepEP README](https://github.com/deepseek-ai/DeepEP/blob/dd758caf451848bd150e1046af3d0a73e5fff38d/README.md), commit `dd758caf...`, 2026-07-14 | separate high-throughput/low-latency MoE dispatch/combine; named prerequisites | explicitly CUDA/NCCL/NVLink/RDMA; benchmark claims are on listed NVIDIA fabrics, not HaloFPX |
| S44-05 | [FasterMoE project/paper record](https://pacman.cs.tsinghua.edu.cn/~zjd/projects/fastermoe/), PPoPP 2022, DOI `10.1145/3503221.3508418` | performance-model-guided shadowing of popular experts | training-focused, many GPUs, different stack; its thresholds/results cannot be reused |
| S44-06 | [DeepSeekMoE](https://arxiv.org/abs/2401.06066), arXiv:2401.06066, submitted 2024-01-11 | fine-grained routed experts and shared-expert isolation | architecture/training paper; target checkpoints may implement different dimensions/routing details |
| S44-07 | [Switch Transformers](https://www.jmlr.org/papers/v23/21-0998.html), JMLR 23(120), 2022 | top-1 routing, fixed capacity, overflow trade-off | TPU training design; dropping semantics must not be assumed for inference models |
| S44-08 | [FlexMoE](https://arxiv.org/abs/2304.03946), arXiv:2304.03946, submitted 2023-04-08, manuscript dated November 2022 | dynamic monitoring, expand/shrink/migrate, slowest-rank cost model | training workloads and other GPUs; observed skew is not universal |
| S44-09 | [PyTorch distributed collectives](https://docs.pytorch.org/docs/2.12/distributed.html), documentation version 2.12, accessed 2026-07-16 | `all_to_all_single`, unequal split sizes, process-group semantics | API semantics only; backend support/performance varies |
| S44-10 | [HaloFPX section 34](../../06_Models_Quantization_and_Inference/34_MoE_Routing_Expert_Telemetry_and_Expert_Placement_Inputs/sources.md), verified 2026-07-16 | pinned llama.cpp router/graph evidence and telemetry gap | internal synthesis; follow its primary links for implementation claims |

## Contradictions and applicability notes

- **[VERIFIED]** vLLM, Megatron, and DeepEP show that modern serving/training stacks support expert parallel patterns; **[VERIFIED]** pinned ROCmFPX does not expose the corresponding HaloFPX control plane [S44-01][S44-02][S44-03][S44-04]. This is a feature gap, not proof of incompatibility.
- **[INFERENCE]** Training papers establish that expert skew and replication can matter, but they disagree in mechanism and operate at different scales. HaloFPX must measure inference traces rather than import their replica policies.
- **[OPEN]** No primary source located in this pass provides an end-to-end HIP/Vulkan, two-node, dual-USB4 hot-expert replication result.

## Source freshness triggers

Recheck S44-01 through S44-04 when any pinned runtime commit changes, and re-run E44-01/E44-05 when model bytes, quantization, backend, transport, or hardware revision changes.
