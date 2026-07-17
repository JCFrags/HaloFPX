---
section_id: "36"
title: "Speculative decoding open questions"
status: "needs-machine-validation"
last_verified: "2026-07-16"
applies_to:
  repositories: []
  software_versions: []
  hardware_revisions: ["dual Strix Halo"]
related_sections: ["29", "41", "45", "52"]
---

# Open questions

| ID | Question | Evidence needed |
|---|---|---|
| O36-01 | Which target models have compatible native MTP weights? | exact model/GGUF hashes and load tests |
| O36-02 | Which external draft models share the exact tokenizer/vocabulary contract? | metadata comparison and adversarial token tests |
| O36-03 | Does current `draft-mtp` preserve the requested sampling distribution for every sampler? | source review and statistical tests |
| O36-04 | What draft depth wins per workload/concurrency? | M36-01 |
| O36-05 | Can node-2 drafting overlap enough work to amortize dual-link latency? | timeline trace and M36-01 |
| O36-06 | How are grammar/tool/multimodal requests handled? | feature matrix and equality tests |
| O36-07 | What speculative state is persisted, and at which layer? | checkpoint design plus M36-02 |
| O36-08 | What happens when target and draft quantizations differ materially? | quality/acceptance matrix |
| O36-09 | Which ROCmFPX MTP changes are absent from upstream/current base? | commit-level lineage diff |

