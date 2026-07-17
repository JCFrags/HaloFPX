---
section_id: "29"
title: "Model catalog open questions"
status: "needs-machine-validation"
last_verified: "2026-07-17"
applies_to:
  repositories: ["charlie12345/ROCmFPX"]
  software_versions: ["a5605a7"]
  hardware_revisions: []
related_sections: ["31", "33", "35", "36"]
---

# Open questions

| ID | Question / evidence needed | Closure evidence |
|---|---|---|
| OQ29-01 | Is a modern pure-MHA production baseline required? | workload requirement plus pinned checkpoint |
| OQ29-02 | Which sliding-window/global model is the canonical validation target? | pinned config/license and fit estimate |
| OQ29-03 | Which exact model revisions are already stored locally? | hash inventory from both machines |
| OQ29-04 | Which candidates convert cleanly at ROCmFPX `a5605a7`? | converter logs and tensor audits |
| OQ29-05 | Which architecture/backend combinations execute entirely on HIP and Vulkan? | backend-op traces and fallback report |
| OQ29-06 | What context and slot counts fit each candidate at each cache type? | matched memory sweeps on both nodes |
| OQ29-07 | Does DeepSeek-V3 have an acceptable two-node placement, or should a smaller MLA/MTP model replace it? | explicit rank plan and measured memory/transport budget |
| OQ29-08 | What recurrent state must be serialized for Nemotron-H and how is partial rollback handled? | state schema plus deterministic restore tests |
| OQ29-09 | **[OPEN]** Which evidence fills every required catalog field per target: total/active parameters, architecture dimensions, actual quant artifact sizes, KV/special-state bytes, exact license, conversion status, backend support, and validation stage? | schema-complete catalog rows linked to immutable sources and machine evidence |
