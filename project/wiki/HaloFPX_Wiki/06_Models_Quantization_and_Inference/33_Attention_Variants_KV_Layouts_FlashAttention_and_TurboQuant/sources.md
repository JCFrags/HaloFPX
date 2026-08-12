---
section_id: "33"
title: "Attention and KV sources"
status: "verified"
last_verified: "2026-07-19"
applies_to:
  repositories: ["ggml-org/llama.cpp", "charlie12345/ROCmFPX"]
  software_versions: ["788e07d", "a5605a7"]
  hardware_revisions: []
related_sections: ["30", "35"]
---

# Sources

| ID | Primary source | Supports | Limitation |
|---|---|---|---|
| S33-01 | [upstream cache-type parser](https://github.com/ggml-org/llama.cpp/blob/788e07dc91d266ad3162a1ce9037665656269689/common/arg.cpp), commit `788e07d`, accessed 2026-07-16 | exact accepted upstream K/V types and FA CLI | parsing does not prove every backend/shape |
| S33-02 | [ROCmFPX cache-type parser](https://github.com/charlie12345/ROCmFPX/blob/a5605a72768c6562241b248e268e33dc92787394/common/arg.cpp), [custom type registry](https://github.com/charlie12345/ROCmFPX/blob/a5605a72768c6562241b248e268e33dc92787394/ggml/include/ggml.h), accessed 2026-07-16 | fork-only cache types and numeric IDs | fork-only, not upstream-compatible by default |
| S33-03 | [upstream KV cache implementation](https://github.com/ggml-org/llama.cpp/blob/788e07dc91d266ad3162a1ce9037665656269689/src/llama-kv-cache.cpp), accessed 2026-07-16 | allocation, views, size logs, rotation and shift constraints | architecture paths must be traced |
| S33-04 | [FlashAttention paper](https://arxiv.org/abs/2205.14135), Dao et al., 2022 | IO-aware exact tiled attention concept | paper GPU results do not establish gfx1151 support |
| S33-05 | [TurboQuant paper](https://arxiv.org/abs/2504.19874), Zandieh et al., ICLR 2026 | online vector quantization method | paper results do not establish fork correctness/performance |
| S33-06 | [ROCmFPX serving guide](https://github.com/charlie12345/ROCmFPX/blob/a5605a72768c6562241b248e268e33dc92787394/docs/ROCmFPX-SERVING.md), [fork KV implementation](https://github.com/charlie12345/ROCmFPX/blob/a5605a72768c6562241b248e268e33dc92787394/src/llama-kv-cache.cpp), accessed 2026-07-16 | asymmetric TurboQuant and boundary policy in fork | other-machine measurements; revalidation required |
| S33-07 | [GQA paper](https://aclanthology.org/2023.emnlp-main.298/), Ainslie et al., EMNLP 2023 | grouped-query definition | not implementation-specific |
| S33-08 | [DeepSeek-V2 paper](https://arxiv.org/abs/2405.04434), DeepSeek-AI, 2024 | MLA architecture and compressed latent cache rationale | exact GGUF graph remains source-specific |
| S33-09 | [Nathanw1014 combined branch](https://github.com/Nathanw1014/llama.cpp/tree/a18067a85e986f7798f43d98345ed5b86b55cf88) and [design/benchmark documentation](https://github.com/Nathanw1014/llama.cpp/tree/a18067a85e986f7798f43d98345ed5b86b55cf88/docs/fa-quant-kv-gqa), preserved 2026-07-18 | exact Vulkan/HIP implementation, tests, raw Qwen receipts, limitations, and branch history | fork measurements are not local HaloFPX results; combined branch contains later documentation/CI commits |
| S33-10 | [Strix Halo Reddit report](https://www.reddit.com/r/StrixHalo/comments/1uzqg5m/i_made_quantized_kv_cache_workable_on_strix_halo/), accessed 2026-07-18 | author explanation and independent MiniMax M2.7 230B Vulkan reproduction with context-specific pp/tg and scratch observations | secondary, self-reported, non-archival; local snapshot was blocked; exact local hardware/software controls require reproduction |
| S33-11 | [upstream Vulkan PR #25494](https://github.com/ggml-org/llama.cpp/pull/25494), metadata/patch captured 2026-07-18; [local intake receipt](../../../../sources/repositories/candidate-intake/2026-07-18-strix-halo-quant-kv/README.md) | open upstream status, review discussion, exact snapshots/hashes, complete Git bundle, and failed ROCmFPX apply-check record | PR is open and unreviewed; HIP lane is not an upstream proposal; neither patch is approved for HaloFPX |
| S33-12 | HaloFPX L14Q-T01 anchor commit `37ff5e4f6ab48ed7d8b0ea2fda05a6304091ae2b` (tree `921dd1709ab3ee343416d0d3137f46059eef6e6b`), accepted independent promotion review, and exact receipt SHA-256 `31b117f206bdf598f69ffa4a4000d7a73b557e976a251ba5f3971b061475ff07` under `C:/Users/britt/Documents/HaloFPX/docs/halofpx`, verified 2026-07-19 | target-native test-only 20-case Q8_0/Q4_0 coverage at head dimensions 128/256, KV 255/256/257, GQA ratio 8, and multi-batch; explicit ROCm h160 unsupported control; per-node CPU/ROCm/Vulkan 200/200 focused executions and zero-failure inherited full inventories; P3/no-copy/similarity review; raw node bundles `f03ba3e2ad6fb61e00de788362e653ee166c7b1c7ba4e43d49a372ef795a1e2f` and `5e424f1e11312f00c10b40467ee1d9c25b12aa3746a74459714045144619de21` | no runtime, kernel, selector, routing, CMake, CLI, service, deployment, persistence, performance, memory, speedup, or zero-regression claim; donor runtime lanes remain unadmitted; CRLF launcher artifacts are retained but excluded; standard runtime K/V types only, not ROCmFPX K/V formats |
