---
section_id: "32"
title: "llama.cpp Model Loading, Graph Construction, and Backend Lifecycle"
status: "needs-machine-validation"
last_verified: "2026-07-16"
applies_to:
  repositories: ["ggml-org/llama.cpp", "charlie12345/ROCmFPX"]
  software_versions: ["llama.cpp 788e07d", "ROCmFPX a5605a7"]
  hardware_revisions: []
related_sections: ["33", "39", "41", "57", "61", "68"]
---

# Engine lifecycle

At the pinned upstream commit, the engine lifecycle is:

`backend registration -> GGUF/model load -> tensor buffer placement -> context/KV/recurrent-state allocation -> graph reserve -> tokenize/batch -> graph build -> scheduler split/allocate/execute -> logits/embeddings -> sampling -> server slot update -> optional state serialization`.

**[VERIFIED]** These stages are separate source modules rather than one stable plugin interface [S32-01, S32-02, S32-03, S32-04, S32-05]. **[RECOMMENDATION]** HaloFPX should first add observability and narrow adapter seams; persistent distributed graphs or external buffers must not bypass upstream scheduler/KV invariants.

## Research split

- Completed now: pinned call-path/source map and candidate integration seams.
- Machine work: trace actual model load/placement, graph reuse, scheduler splits, state bytes, server slots, backends and errors on both nodes.
- Contingent: rank graph lifetime, distributed buffer ABI, persistent cache restore hook, and failure/fallback policy.
