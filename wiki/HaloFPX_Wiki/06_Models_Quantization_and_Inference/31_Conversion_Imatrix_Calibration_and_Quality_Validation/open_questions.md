---
section_id: "31"
title: "Conversion and validation open questions"
status: "needs-machine-validation"
last_verified: "2026-07-16"
applies_to:
  repositories: ["charlie12345/ROCmFPX"]
  software_versions: ["a5605a7"]
  hardware_revisions: []
related_sections: ["29", "30", "78"]
---

# Open questions

| ID | Question | Closure evidence |
|---|---|---|
| OQ31-01 | Which exact source checkpoints are licensed and approved? | immutable inventory and license review |
| OQ31-02 | What calibration mixture and token budget represent local workloads? | versioned corpus manifest and coverage review |
| OQ31-03 | What is normal source-vs-BF16 numeric variance per architecture/backend? | repeated logit comparisons |
| OQ31-04 | Are proposed PPL/KLD/task thresholds strict enough? | baseline variance and stakeholder approval |
| OQ31-05 | Which tensors should exclude/include imatrix per architecture? | controlled ablation |
| OQ31-06 | How much RAM/disk is required for each actual source and candidate set? | dry-run and observed peak metrics |
| OQ31-07 | Do interrupted imatrix chunk merges avoid overlap and preserve dataset metadata? | synthetic resume/merge test |
| OQ31-08 | Which public evaluation corpora are redistributable in the workspace? | license/provenance review |
| OQ31-09 | How will multimodal, recurrent, MoE-router, and MTP-head quality be gated? | family-specific test definitions |

