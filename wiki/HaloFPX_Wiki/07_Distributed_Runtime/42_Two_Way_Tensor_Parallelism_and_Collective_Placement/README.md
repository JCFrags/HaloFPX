---
section_id: "42"
title: "Two-Way Tensor Parallelism and Collective Placement"
status: "needs-machine-validation"
last_verified: "2026-07-16"
applies_to:
  repositories: ["NVIDIA/Megatron-LM@740c16e6b80a753bea26232148d9bb2d7f0c827a", "ggml-org/llama.cpp@788e07dc91d266ad3162a1ce9037665656269689", "ROCm/rccl@57e58688f44c77076ad536ef1f6b68741fc6e694"]
  software_versions: ["RCCL docs 2.30.4"]
  hardware_revisions: ["dual Strix Halo premise"]
related_sections: ["30", "31", "32", "38", "39", "44", "45", "48", "51", "52"]
---

# 42 - Two-Way Tensor Parallelism and Collective Placement

**[RECOMMENDATION]** Prototype Megatron-style two-rank column/row tensor parallelism as a distinct HaloFPX mode: shard Q/FFN expansion projections by output features, keep each rank's attention/activation local, shard attention-output/FFN-down projections by input features, then sum their hidden-size partials with two all-reduces per ordinary transformer block.

This is not what current `llama.cpp --tensor-split` proves. Its RPC path exposes remote devices and tensor placement/offload; it is documented as proof-of-concept and does not establish the proposed rank-synchronous TP algorithm.

Pages: [facts and mapping](facts_and_constraints.md), [design](design_implications.md), [checks](procedures_and_checks.md), [questions](open_questions.md), [sources](sources.md).
