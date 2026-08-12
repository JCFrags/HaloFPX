---
section_id: "30"
title: "ROCmFPX format open questions"
status: "needs-machine-validation"
last_verified: "2026-07-16"
applies_to:
  repositories: ["charlie12345/ROCmFPX"]
  software_versions: ["a5605a7"]
  hardware_revisions: []
related_sections: ["31", "33", "37"]
---

# Open questions

| ID | Question | Required evidence |
|---|---|---|
| OQ30-01 | Which exact recipe is default per target architecture? | matched source/BF16/quant quality and speed |
| OQ30-02 | Do all family ops pass on both local HIP and Vulkan builds? | raw backend-op logs |
| OQ30-03 | Are FP2 and ROCmFP weight cache types intended product features or research-only? | maintainer decision plus quality/backend matrix |
| OQ30-04 | Which tensors need protection for MLA, recurrent, multimodal, and MTP models? | ablation by tensor category |
| OQ30-05 | Does imatrix improve each ROCmFPX recipe on representative local workloads? | calibrated A/B runs |
| OQ30-06 | What exact custom-type compatibility/version policy will GGUF and persistent cache use? | format ADR and rejection tests |
| OQ30-07 | Can upstream changes be rebased without renumbering/colliding custom GGML types? | merge audit against pinned upstream |

