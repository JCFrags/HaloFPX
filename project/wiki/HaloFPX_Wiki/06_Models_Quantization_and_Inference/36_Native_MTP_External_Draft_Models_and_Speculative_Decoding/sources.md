---
section_id: "36"
title: "Speculative decoding sources"
status: "draft"
last_verified: "2026-07-16"
applies_to:
  repositories: ["ggml-org/llama.cpp@788e07dc91d266ad3162a1ce9037665656269689", "charlie12345/ROCmFPX@a5605a72768c6562241b248e268e33dc92787394"]
  software_versions: []
  hardware_revisions: []
related_sections: ["41", "76"]
---

# Sources

| ID | Primary source and revision | Supports | Limitations |
|---|---|---|---|
| S36-01 | [llama.cpp speculative documentation](https://github.com/ggml-org/llama.cpp/blob/788e07dc91d266ad3162a1ce9037665656269689/docs/speculative.md), commit `788e07dc`, committed 2026-07-17 +02:00, accessed 2026-07-16 PDT | modes and CLI surface | documentation can lag code |
| S36-02 | [llama.cpp speculative implementation](https://github.com/ggml-org/llama.cpp/blob/788e07dc91d266ad3162a1ce9037665656269689/common/speculative.cpp), commit `788e07dc` | controller contexts/counters/acceptance path | runtime performance unmeasured |
| S36-03 | [Fast Inference via Speculative Decoding](https://proceedings.mlr.press/v202/leviathan23a.html), ICML 2023 | lossless acceptance/correction algorithm | implementation must match algorithm |
| S36-04 | [DeepSeek-V3 Technical Report](https://arxiv.org/abs/2412.19437), arXiv:2412.19437, 2024-12-27 | MTP training objective/architecture | vendor paper; not local performance |
| S36-05 | [DeepSeek-V3 repository](https://github.com/deepseek-ai/DeepSeek-V3/tree/9b4e9788e4a3a731f7567338ed15d3ec549ce03b), commit `9b4e9788`, accessed 2026-07-16 | published main/MTP weight accounting | repository documents original V3, not every later MTP format |
| S36-06 | [ROCmFPX source and MTP experiment log](https://github.com/charlie12345/ROCmFPX/tree/a5605a72768c6562241b248e268e33dc92787394), commit `a5605a72`, committed 2026-07-16 -04:00, accessed 2026-07-16 PDT | fork-specific MTP and gfx1151 experiment leads | repository measurements are environment-scoped |

Source count is 6.
