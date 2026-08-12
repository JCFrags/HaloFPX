---
section_id: "43"
title: "Pipeline Parallel Sources"
status: "draft"
last_verified: "2026-07-16"
applies_to:
  repositories: ["ROCmFPX", "llama.cpp", "vLLM", "PyTorch"]
  software_versions: ["PyTorch distributed.pipelining 2.13 documentation"]
  hardware_revisions: []
related_sections: ["32", "38", "39", "42", "45", "46", "48", "51"]
---

# Sources

All web and repository sources were accessed 2026-07-16 PT. Repository heads were observed with `git ls-remote`; code claims were checked in shallow clones at the listed commits.

| ID | Primary source and revision | Claims supported | Limitations or conflicts |
|---|---|---|---|
| S43-01 | [ROCmFPX server options](https://github.com/charlie12345/ROCmFPX/blob/a5605a72768c6562241b248e268e33dc92787394/tools/server/README.md), [`llama-model.cpp`](https://github.com/charlie12345/ROCmFPX/blob/a5605a72768c6562241b248e268e33dc92787394/src/llama-model.cpp), and [`llama-context.cpp`](https://github.com/charlie12345/ROCmFPX/blob/a5605a72768c6562241b248e268e33dc92787394/src/llama-context.cpp), commit `a5605a72768c6562241b248e268e33dc92787394`, authored 2026-07-16 | layer/KV split description, cumulative layer placement, scheduler prerequisites/fallback, graph-reuse synchronization | candidate base is moving; code behavior is not two-host performance proof |
| S43-02 | [ROCmFPX RPC README](https://github.com/charlie12345/ROCmFPX/blob/a5605a72768c6562241b248e268e33dc92787394/tools/rpc/README.md), same commit/date | remote-device offload, default weight/KV distribution, local tensor cache, proof-of-concept/security warning | current RPC is explicitly fragile/insecure; no custom dual-link guarantee |
| S43-03 | [llama.cpp server options](https://github.com/ggml-org/llama.cpp/blob/788e07dc91d266ad3162a1ce9037665656269689/tools/server/README.md), [`llama-model.cpp`](https://github.com/ggml-org/llama.cpp/blob/788e07dc91d266ad3162a1ce9037665656269689/src/llama-model.cpp), [`llama-context.cpp`](https://github.com/ggml-org/llama.cpp/blob/788e07dc91d266ad3162a1ce9037665656269689/src/llama-context.cpp), and [RPC README](https://github.com/ggml-org/llama.cpp/blob/788e07dc91d266ad3162a1ce9037665656269689/tools/rpc/README.md), commit `788e07dc91d266ad3162a1ce9037665656269689`, authored 2026-07-17 in author timezone | upstream layer placement, scheduler conditions/memory cost, RPC status | upstream is a comparison baseline; author date is one day ahead of PT access date |
| S43-04 | [vLLM contiguous PP indices](https://github.com/vllm-project/vllm/blob/9354f222042986addf20709e5274fc26e0d09745/vllm/distributed/utils.py), [model layer construction](https://github.com/vllm-project/vllm/blob/9354f222042986addf20709e5274fc26e0d09745/vllm/model_executor/models/utils.py), [GPU model runner](https://github.com/vllm-project/vllm/blob/9354f222042986addf20709e5274fc26e0d09745/vllm/v1/worker/gpu_model_runner.py), and [versioned serving guide](https://docs.vllm.ai/en/v0.17.0/serving/parallelism_scaling/), code commit `9354f222042986addf20709e5274fc26e0d09745`, authored 2026-07-17 in author timezone; docs release `v0.17.0` at commit `b31e9326a7d9394aab8c767f8ebe225c65594b60`, 2026-03-06 | contiguous/manual splits, embedding/head balancing, supported-model gate, intermediate/final-stage behavior, PP capacity guidance | PyTorch-oriented implementation and datacenter examples; not a HaloFPX dependency or measurement |
| S43-05 | [PyTorch 2.13 Pipeline Parallelism documentation](https://docs.pytorch.org/docs/2.13/distributed.pipelining.html), release tag `v2.13.0` at commit `cf30153c4c131c8164ee7798e5022d810682e2cb`, 2026-07-03 | stage abstraction, microbatch split, fill-drain schedule, static shape contract, inference applicability | API is alpha and PyTorch/CUDA-oriented examples do not establish ROCmFPX integration |
| S43-06 | [Sarathi: Efficient LLM Inference by Piggybacking Decodes with Chunked Prefills](https://arxiv.org/abs/2308.16369), Agrawal et al., arXiv `2308.16369`, submitted 2023-08-31 | inference-specific pipeline bubbles, chunked prefill, decode-maximal batching | reported NVIDIA-cluster results are not portable measurements |
| S43-07 | [Orca: A Distributed Serving System for Transformer-Based Generative Models](https://www.usenix.org/conference/osdi22/presentation/yu), Yu et al., USENIX OSDI 2022, July 2022 | autoregressive dependency, iteration-level scheduling, selective batching | system architecture/hardware differ from HaloFPX |
| S43-08 | [GPipe: Efficient Training of Giant Neural Networks using Pipeline Parallelism](https://arxiv.org/abs/1811.06965), Huang et al., arXiv `1811.06965`, submitted 2018-11-16 | canonical microbatch/fill-drain pipeline model | training evidence; used only for scheduling arithmetic, not inference performance |
| S43-09 | Local path `C:\Users\britt\Documents\Agent_Harness\guide\architecture.md`, read 2026-07-16 | evidence promotion, provenance, and review discipline | governance source only; no runtime claim |

## Known conflicts and gaps

- ROCmFPX/llama.cpp call layer mode pipelined, but this does not by itself define a service-level microbatch schedule, rank-local loader, or recoverable two-process protocol.
- vLLM demonstrates supported pipeline serving and manual layer counts, but its code and transport stack are not the HaloFPX implementation target.
- Published scheduling results use different accelerators and interconnects. Only `DR-43-E1` through `DR-43-E5` may establish HaloFPX measurements.
- The PyTorch stable documentation resolved to version 2.13 on the access date; re-pin the exact installed runtime if PyTorch code is adopted.
