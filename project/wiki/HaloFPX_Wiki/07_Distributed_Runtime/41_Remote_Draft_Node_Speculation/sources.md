---
section_id: "41"
title: "Remote Speculation Sources"
status: "draft"
last_verified: "2026-07-16"
applies_to:
  repositories: ["llama.cpp", "vLLM"]
  software_versions: []
  hardware_revisions: []
related_sections: ["31", "38", "48"]
---

# Sources

| ID | Source/revision | Supports | Limitation |
|---|---|---|---|
| S41-01 | [Fast Inference from Transformers via Speculative Decoding](https://arxiv.org/abs/2211.17192), Leviathan, Kalman, Matias, arXiv `2211.17192`, 2022-11-30 / ICML 2023 | exact accept/reject/correction semantics | not remote-system measurement |
| S41-02 | [Accelerating Large Language Model Decoding with Speculative Sampling](https://arxiv.org/abs/2302.01318), Chen et al., arXiv `2302.01318`, 2023-02-02 | lossless speculative sampling and system framing | not HaloFPX/fabric evidence |
| S41-03 | [llama.cpp speculative decoding](https://github.com/ggml-org/llama.cpp/blob/788e07dc91d266ad3162a1ce9037665656269689/docs/speculative.md) and [`common/speculative.cpp`](https://github.com/ggml-org/llama.cpp/blob/788e07dc91d266ad3162a1ce9037665656269689/common/speculative.cpp), commit `788e07d`; accessed 2026-07-16 | implemented variants/options/source | local runtime, fast-moving |
| S41-04 | [vLLM draft-model docs](https://github.com/vllm-project/vllm/blob/9354f222042986addf20709e5274fc26e0d09745/docs/features/speculative_decoding/draft_model.md), commit `9354f22`; accessed 2026-07-16 | draft configuration and heterogeneous-vocab constraint | framework-specific |
| S41-05 | [vLLM MTP docs](https://github.com/vllm-project/vllm/blob/9354f222042986addf20709e5274fc26e0d09745/docs/features/speculative_decoding/mtp.md), commit `9354f22`; accessed 2026-07-16 | MTP distinction and support constraints | support changes rapidly |
| S41-06 | [ROCmFPX commit `a5605a7`](https://github.com/charlie12345/ROCmFPX/tree/a5605a72768c6562241b248e268e33dc92787394), accessed 2026-07-16 | candidate implementation base contains speculation paths | remote design not established |
