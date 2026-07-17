---
section_id: "34"
title: "MoE sources"
status: "draft"
last_verified: "2026-07-16"
applies_to:
  repositories: ["ggml-org/llama.cpp@788e07dc91d266ad3162a1ce9037665656269689", "fewtarius/CachyLLama@6be745998f568e379ea197fcf827baec73ff9940"]
  software_versions: []
  hardware_revisions: []
related_sections: ["29", "44"]
---

# Sources

| ID | Primary source and revision | Supports | Limitations |
|---|---|---|---|
| S34-01 | [llama.cpp source](https://github.com/ggml-org/llama.cpp/tree/788e07dc91d266ad3162a1ce9037665656269689), commit `788e07dc`, committed 2026-07-17 +02:00, accessed 2026-07-16 PDT | current graph, metadata, tensor shapes, top-k | moving project; backend performance unmeasured |
| S34-02 | [CachyLLama source](https://github.com/fewtarius/CachyLLama/tree/6be745998f568e379ea197fcf827baec73ff9940), commit `6be74599`, committed 2026-07-08 -04:00, accessed 2026-07-16 | absence of identifiable telemetry in inspected tree | absence claim is search-scoped; branches not exhausted |
| S34-03 | [Mixtral of Experts](https://arxiv.org/abs/2401.04088), arXiv:2401.04088, 2024-01-08 | top-2 of eight, active-parameter framing | model-specific; paper claims are not local measurements |
| S34-04 | [DeepSeek-V2](https://arxiv.org/abs/2405.04434), arXiv:2405.04434, 2024-05 | shared and routed experts | training architecture, not HaloFPX runtime evidence |
| S34-05 | [DeepSeek-V3 Technical Report](https://arxiv.org/abs/2412.19437), arXiv:2412.19437, 2024-12-27 | total/active parameters, balancing strategy | vendor-authored; not an inference trace |
| S34-06 | [llama.cpp issue 20757](https://github.com/ggml-org/llama.cpp/issues/20757), opened 2026 | candidate expert-cache hooks/telemetry ideas | issue/user measurements; not verified behavior |

Source count is 6. Repository statements and issue measurements are not promoted beyond their stated scope.
