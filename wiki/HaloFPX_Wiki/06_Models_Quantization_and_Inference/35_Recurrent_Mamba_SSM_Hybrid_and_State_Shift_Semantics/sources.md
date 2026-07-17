---
section_id: "35"
title: "Recurrent state sources"
status: "draft"
last_verified: "2026-07-16"
applies_to:
  repositories: ["ggml-org/llama.cpp@788e07dc91d266ad3162a1ce9037665656269689", "fewtarius/CachyLLama@6be745998f568e379ea197fcf827baec73ff9940"]
  software_versions: []
  hardware_revisions: []
related_sections: ["61", "77"]
---

# Sources

| ID | Primary source and revision | Supports | Limitations |
|---|---|---|---|
| S35-01 | [llama.cpp source](https://github.com/ggml-org/llama.cpp/tree/788e07dc91d266ad3162a1ce9037665656269689), commit `788e07dc`, committed 2026-07-17 +02:00, accessed 2026-07-16 PDT | recurrent memory, sequence/state APIs | source semantics need runtime tests |
| S35-02 | [CachyLLama source](https://github.com/fewtarius/CachyLLama/tree/6be745998f568e379ea197fcf827baec73ff9940), commit `6be74599`, committed 2026-07-08 -04:00, accessed 2026-07-16 | fork's recurrent/hybrid and SSD-cache integration surface | no portability guarantee |
| S35-03 | [Mamba](https://arxiv.org/abs/2312.00752), arXiv:2312.00752v2, 2024-05-31 | selective SSM recurrence and scan | architecture paper, not GGUF state schema |
| S35-04 | [Jamba](https://arxiv.org/abs/2403.19887), arXiv:2403.19887, 2024 | hybrid Transformer-Mamba-MoE architecture | model-specific and training-oriented |
| S35-05 | [llama.cpp issue 21681](https://github.com/ggml-org/llama.cpp/issues/21681), opened 2026 | hybrid prompt-cache drift reproduction lead | unconfirmed issue, not universal behavior |
| S35-06 | [llama.cpp public API](https://github.com/ggml-org/llama.cpp/blob/788e07dc91d266ad3162a1ce9037665656269689/include/llama.h), commit `788e07dc` | state/sequence API contract surface | implementation/model support varies |

Source count is 6.
