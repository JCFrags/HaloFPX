---
section_id: "76"
title: "Distributed Benchmark Sources"
status: "needs-machine-validation"
last_verified: "2026-07-16"
applies_to:
  repositories:
    - "ggml-org/llama.cpp@788e07dc91d266ad3162a1ce9037665656269689"
    - "charlie12345/ROCmFPX@a5605a72768c6562241b248e268e33dc92787394"
  software_versions: []
  hardware_revisions: []
related_sections: ["38", "73", "75"]
---

# Sources

Accessed 2026-07-16. Published performance remains scoped to its original systems.

## S76-001 — llama.cpp RPC documentation

- **Repository/revision:** ggml-org/llama.cpp, commit `788e07dc91d266ad3162a1ce9037665656269689`.
- **URL:** https://github.com/ggml-org/llama.cpp/blob/788e07dc91d266ad3162a1ce9037665656269689/tools/rpc/README.md
- **Supports:** RPC device exposure, tensor split, local cache, proof-of-concept/security warning.
- **Limit:** documents available mechanisms, not HaloFPX modes or performance.

## S76-002 — llama.cpp speculative decoding documentation

- **Repository/revision:** ggml-org/llama.cpp, commit `788e07dc91d266ad3162a1ce9037665656269689`.
- **URL:** https://github.com/ggml-org/llama.cpp/blob/788e07dc91d266ad3162a1ce9037665656269689/docs/speculative.md
- **Supports:** draft-model, MTP, n-gram modes and controls.
- **Limit:** exact target model support and benchmark integration require source/machine checks.

## S76-003 — Fast Inference via Speculative Decoding

- **Publisher/revision:** Leviathan, Kalman, Matias; arXiv:2211.17192, submitted 2022-11-30, revised 2023-05-24.
- **URL:** https://arxiv.org/abs/2211.17192
- **Supports:** exact-distribution speculative method and acceptance/cost motivation.
- **Limit:** reported systems/models are not HaloFPX.

## S76-004 — Megatron-LM model parallelism

- **Publisher/revision:** Shoeybi et al.; arXiv:1909.08053, 2019.
- **URL:** https://arxiv.org/abs/1909.08053
- **Supports:** tensor-parallel transformer partitioning and communication placement.
- **Limit:** training-focused NVIDIA environment; no Strix Halo inference prediction.

## S76-005 — GPipe

- **Publisher/revision:** Huang et al.; NeurIPS 2019, paper ID 8305.
- **URL:** https://proceedings.neurips.cc/paper/2019/hash/093f65e080a295f8076b1c5722a46aa2-Abstract.html
- **Supports:** pipeline partitioning, microbatching, and bubble terminology.
- **Limit:** synchronous training design; inference behavior must be measured separately.

## S76-006 — DeepSpeed-MoE

- **Publisher/revision:** Rajbhandari et al.; arXiv:2201.05596, submitted 2022-01-14.
- **URL:** https://arxiv.org/abs/2201.05596
- **Supports:** MoE inference placement/communication as a distinct systems problem.
- **Limit:** published hardware/software and results are not applicable measurements.

## S76-007 — ROCmFPX frozen baseline

- **Repository/revision:** charlie12345/ROCmFPX, commit `a5605a72768c6562241b248e268e33dc92787394`, observed 2026-07-16.
- **URL:** https://github.com/charlie12345/ROCmFPX/tree/a5605a72768c6562241b248e268e33dc92787394
- **Supports:** intended source observation point for future implementation audit.
- **Limit:** no claim that every proposed distributed mode exists.
