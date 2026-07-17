---
section_id: "09"
title: "Functional Requirements, SLOs, and Acceptance Criteria"
status: "needs-machine-validation"
last_verified: "2026-07-16"
applies_to:
  repositories: ["HaloFPX project", "ggml-org/llama.cpp", "charlie12345/ROCmFPX", "fewtarius/CachyLLama"]
  software_versions: ["commits in sources.md"]
  hardware_revisions: ["two matched AMD Strix Halo systems; exact BOM open"]
related_sections: ["06", "07", "08", "10", "38", "48", "54", "60", "69", "71", "78", "80"]
---

# Requirements and acceptance contract

This section is the draft product contract. It distinguishes source-supported inputs from proposed HaloFPX requirements.

- [Requirements catalog](facts_and_constraints.md)
- [SLO definitions and candidate targets](design_implications.md)
- [Acceptance procedure](procedures_and_checks.md)
- [Unratified decisions](open_questions.md)

**[RECOMMENDATION]** A release passes only when every mandatory requirement has linked evidence, every applicable SLO meets its aggregation rule, no correctness/security blocker is open, and the exact build/model/platform manifest is retained.

**[OPEN]** Candidate numeric/relative targets below are engineering starting points, not sponsor-approved commitments and not measured results.

## Research split

- Source research verified relevant baseline server, cache, health, metrics, concurrency, and AMD paths.
- Actual nodes must establish baselines and execute acceptance under cold/warm, single/two-node, load, and fault conditions.
- Final target values, release workload weights, and error budget require owner approval.

