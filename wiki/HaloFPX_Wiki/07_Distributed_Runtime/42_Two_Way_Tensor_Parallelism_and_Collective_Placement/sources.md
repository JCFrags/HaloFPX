---
section_id: "42"
title: "Tensor Parallel Sources"
status: "draft"
last_verified: "2026-07-16"
applies_to:
  repositories: ["Megatron-LM", "RCCL", "llama.cpp", "ROCmFPX"]
  software_versions: ["RCCL docs 2.30.4"]
  hardware_revisions: []
related_sections: ["30", "31", "38", "51"]
---

# Sources

| ID | Source/revision | Supports | Limitation |
|---|---|---|---|
| S42-01 | [Megatron-LM paper](https://arxiv.org/abs/1909.08053), Shoeybi et al., arXiv `1909.08053`, 2019-09-17 | attention-head and MLP intra-layer parallel scheme | training/datacenter GPUs, not HaloFPX measurement |
| S42-02 | [Megatron-LM tensor-parallel layers](https://github.com/NVIDIA/Megatron-LM/blob/740c16e6b80a753bea26232148d9bb2d7f0c827a/megatron/core/tensor_parallel/layers.py) and [mappings](https://github.com/NVIDIA/Megatron-LM/blob/740c16e6b80a753bea26232148d9bb2d7f0c827a/megatron/core/tensor_parallel/mappings.py), commit `740c16e`, 2026-07-16; accessed 2026-07-16 | current row/column primitives and collective mappings | PyTorch/CUDA implementation analogue |
| S42-03 | [AMD RCCL overview](https://rocm.docs.amd.com/projects/rccl/en/develop/what-is-rccl.html) and [API](https://rocm.docs.amd.com/projects/rccl/en/develop/api-reference/api-library.html), docs `2.30.4`; source [commit `57e5868`](https://github.com/ROCm/rccl/tree/57e58688f44c77076ad536ef1f6b68741fc6e694); accessed 2026-07-16 | available collective primitives and algorithm claims | intended fabric/backend compatibility unproven |
| S42-04 | [llama.cpp RPC README](https://github.com/ggml-org/llama.cpp/blob/788e07dc91d266ad3162a1ce9037665656269689/tools/rpc/README.md), commit `788e07d`; accessed 2026-07-16 | actual RPC/tensor-split status | not Megatron TP; proof-of-concept |
| S42-05 | [ROCmFPX commit `a5605a7`](https://github.com/charlie12345/ROCmFPX/tree/a5605a72768c6562241b248e268e33dc92787394), 2026-07-16; accessed 2026-07-16 | intended base revision | TP design not established |
| S42-06 | [Efficient Large-Scale Language Model Training on GPU Clusters](https://arxiv.org/abs/2104.04473), Narayanan et al., 2021-04-09 | TP/PP/DP tradeoff context | training clusters, not local inference result |
