---
section_id: "34"
title: "MoE facts and constraints"
status: "needs-machine-validation"
last_verified: "2026-07-16"
applies_to:
  repositories: ["ggml-org/llama.cpp@788e07dc91d266ad3162a1ce9037665656269689", "fewtarius/CachyLLama@6be745998f568e379ea197fcf827baec73ff9940"]
  software_versions: []
  hardware_revisions: []
related_sections: ["29", "44"]
---

# Facts and constraints

## Runtime graph

**[VERIFIED]** At commit `788e07d`, `build_moe_ffn()` supports softmax or sigmoid scoring, optional router bias, group-limited routing, top-k through `ggml_argsort_top_k`, optional weight renormalization, and `ggml_mul_mat_id`-style indexed expert operations [S34-01]. Its expert tensors are separate gate/up/down (or fused gate-up) tensors; shared-expert tensors are represented separately in model builders.

**[VERIFIED]** The selected-expert index tensor has shape `[n_expert_used, n_tokens]`; selected weights are gathered and normalized before expert outputs are reduced [S34-01]. Therefore batch routing is token-by-token but expert kernels may process the token batch together.

**[VERIFIED]** llama.cpp's GGUF metadata distinguishes total, used, shared, group, and used-group expert counts. The model loader asserts used experts do not exceed total experts [S34-01]. Warmup can force all experts, so warmup traces must not be mixed with production routing statistics.

## Model patterns

| Pattern | Primary evidence | Routing consequence |
|---|---|---|
| Mixtral 8x7B | two of eight FFN experts per token [S34-03] | top-2, no shared-expert premise from this paper |
| DeepSeek-V2 | shared experts plus routed expert specialization [S34-04] | shared path is always-resident cost; routed path is sparse |
| DeepSeek-V3 | 671B total / 37B active; auxiliary-loss-free balancing [S34-05] | training balance does not prove uniform inference traffic |
| llama.cpp families | score functions, grouping, bias and normalization vary in source [S34-01] | telemetry schema must record routing policy/model fingerprint |

**[INFERENCE]** Approximate routed FFN weight traffic per token is the sum of selected gate/up/down expert tensor bytes, but cache residency and batched reuse determine actual memory traffic. "Active parameters" is therefore a compute-footprint descriptor, not a measured bandwidth number.

## CachyLLama status

**[VERIFIED]** A full-tree search of CachyLLama commit `6be7459` found the inherited MoE graph/model machinery but no named expert-usage histogram, export endpoint, trace format, or placement controller [S34-02]. Absence is scoped to that exact commit and search terms.

**[OPEN]** If telemetry exists out-of-tree, in another branch, or in `llama-ai`, its exact revision and schema still need preservation.

