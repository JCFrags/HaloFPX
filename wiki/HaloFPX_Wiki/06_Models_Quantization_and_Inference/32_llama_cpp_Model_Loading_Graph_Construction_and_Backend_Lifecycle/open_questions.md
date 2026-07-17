---
section_id: "32"
title: "Lifecycle open questions"
status: "needs-machine-validation"
last_verified: "2026-07-16"
applies_to:
  repositories: ["ggml-org/llama.cpp", "charlie12345/ROCmFPX"]
  software_versions: ["788e07d", "a5605a7"]
  hardware_revisions: []
related_sections: ["39", "41", "57", "61", "68"]
---

# Open questions

| ID | Question | Closure evidence |
|---|---|---|
| OQ32-01 | What graph signature fields are sufficient for safe reuse? | traced rebuild/reuse matrix and invalidation tests |
| OQ32-02 | Where do actual target models fall back or copy between CPU/HIP/Vulkan? | tensor/backend and scheduler traces |
| OQ32-03 | Which upstream state fields are serialized for KV, recurrent, MTP, sampler and grammar state? | source audit plus byte-level round trips |
| OQ32-04 | Can rank-local cache restore use public APIs or require internal adapters? | prototype without bypassing invariants |
| OQ32-05 | Which ggml backend/buffer interface should represent USB4 remote memory/compute? | interface spike and failure tests |
| OQ32-06 | How are partial rank failure, cancellation and slot reuse coordinated? | protocol ADR and fault injection |
| OQ32-07 | Does persistent graph reuse materially improve measured latency? | matched trace/benchmark |
| OQ32-08 | What is the safe single-node fallback for each distributed placement mode? | explicit plan and forced-rank-loss test |

