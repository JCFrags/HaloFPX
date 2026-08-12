---
section_id: "35"
title: "Recurrent state open questions"
status: "needs-machine-validation"
last_verified: "2026-07-16"
applies_to:
  repositories: []
  software_versions: []
  hardware_revisions: ["dual Strix Halo"]
related_sections: ["29", "57", "61", "77"]
---

# Open questions

| ID | Question | Evidence needed |
|---|---|---|
| O35-01 | Which recurrent/hybrid target models and exact hashes are required? | section 29 catalog |
| O35-02 | Which llama.cpp state bytes are portable across builds, if any? | version-pair restore matrix |
| O35-03 | Which sequence operations are implemented and correct per architecture? | M35-02 per-model tests |
| O35-04 | Does context shift recompute or mutate recurrent state in each model path? | source trace plus logits comparison |
| O35-05 | What exact state is held by Gated DeltaNet/RWKV/other supported models? | M35-01 inventory and model papers |
| O35-06 | Can state migrate between ranks/backends without numeric drift? | explicit migration experiment |
| O35-07 | Is sampler/RNG state part of HaloKV or a higher session layer? | product decision and replay test |
| O35-08 | Has issue 21681's implicated defect been fixed in the selected base? | exact fix commit and regression |

