---
section_id: "38"
title: "Mode Selection Sources"
status: "draft"
last_verified: "2026-07-17"
applies_to:
  repositories: ["ROCmFPX", "llama.cpp", "vLLM", "Megatron-LM"]
  software_versions: []
  hardware_revisions: []
related_sections: ["40", "41", "42"]
---

# Sources

| ID | Primary source and revision | Supports | Limitation |
|---|---|---|---|
| S38-01 | [ROCmFPX](https://github.com/charlie12345/ROCmFPX/tree/a5605a72768c6562241b248e268e33dc92787394), commit `a5605a7`, 2026-07-16; accessed 2026-07-16 | intended code base and current tree | no HaloFPX measurements; moving fork |
| S38-02 | [llama.cpp RPC README](https://github.com/ggml-org/llama.cpp/blob/788e07dc91d266ad3162a1ce9037665656269689/tools/rpc/README.md), commit `788e07d`, 2026-07-17 author timezone; accessed 2026-07-16 PT | RPC status, offload, split/cache behavior | upstream implementation, not custom TP proof |
| S38-03 | [vLLM data-parallel deployment](https://github.com/vllm-project/vllm/blob/9354f222042986addf20709e5274fc26e0d09745/docs/serving/data_parallel_deployment.md), commit `9354f22`, 2026-07-17 author timezone; accessed 2026-07-16 PT | independent KV, queue/cache-aware routing | CUDA-centric system; design analogue only |
| S38-04 | [Orca](https://www.usenix.org/conference/osdi22/presentation/yu), USENIX OSDI 2022, July 2022; accessed 2026-07-16 | iteration scheduling/selective batching | published cluster differs from HaloFPX |
| S38-05 | [Megatron-LM tensor/pipeline/data parallel paper](https://arxiv.org/abs/2104.04473), arXiv `2104.04473`, 2021-04-09 | parallelism tradeoffs and communication | training/datacenter evidence, not local inference result |
| S38-06 | [Fast Inference from Transformers via Speculative Decoding](https://arxiv.org/abs/2211.17192), arXiv `2211.17192`, 2022-11-30 | exact-distribution speculative decoding | does not establish remote-node break-even |
| S38-07 | Local `C:\Users\britt\Documents\Agent_Harness\guide\architecture.md`, read 2026-07-16 | evidence promotion and scoped recommendations | governance, not runtime evidence |
| S38-L01 | [Live Strix Halo target inventory](../../../../sources/measurements/2026-07-17-strix-halo-live-inventory/README.md), captured 2026-07-17 | current roles, deployed commit, model size, placement flags, service readiness, MPTCP two-subflow state | predecessor runtime; no inference correctness or performance measurement |

Repository heads were observed with `git ls-remote`; commit dates were read from shallow Git objects. No benchmark claims were imported.
