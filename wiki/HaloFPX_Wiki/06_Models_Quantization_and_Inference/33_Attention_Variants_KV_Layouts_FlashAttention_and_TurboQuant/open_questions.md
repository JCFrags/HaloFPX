---
section_id: "33"
title: "Attention and KV open questions"
status: "needs-machine-validation"
last_verified: "2026-07-16"
applies_to:
  repositories: ["ggml-org/llama.cpp", "charlie12345/ROCmFPX"]
  software_versions: ["788e07d", "a5605a7"]
  hardware_revisions: []
related_sections: ["35", "42", "57", "61"]
---

# Open questions

| ID | Question | Closure evidence |
|---|---|---|
| OQ33-01 | What are actual K/V/recurrent bytes and strides per target model/type? | startup logs plus tensor inspection |
| OQ33-02 | Which FA kernels run on gfx1151 for each type/shape, and where does fallback occur? | backend traces and op tests |
| OQ33-03 | Which asymmetric K/V pairs pass quality gates at long context? | matched PPL/KLD/task matrix |
| OQ33-04 | Does TurboQuant 3/4 pass on HIP and Vulkan locally? | fork build and backend-op/full-model tests |
| OQ33-05 | Do boundary-layer protections improve local model quality enough to justify mixed per-layer types? | controlled ablation |
| OQ33-06 | How are MLA latent, decoupled RoPE, and any indexer state represented/persisted? | per-model source/graph and state audit |
| OQ33-07 | Does sliding-window allocation physically bound cache memory in each target implementation? | context sweep and cache logs |
| OQ33-08 | Which models safely support cache shifting, and what is the fallback when they do not? | shift tests and explicit server policy |
| OQ33-09 | What GQA/MLA rank ownership minimizes dual-link traffic without duplicating state? | cost model plus measured placement sweep |

