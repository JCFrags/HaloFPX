---
section_id: "81"
title: "CI, Release Gate, Reproducibility, and Regression Open Questions"
status: "needs-machine-validation"
last_verified: "2026-07-17"
applies_to:
  repositories: ["ggml-org/llama.cpp@788e07dc91d266ad3162a1ce9037665656269689", "charlie12345/ROCmFPX@a5605a72768c6562241b248e268e33dc92787394", "fewtarius/CachyLLama@6be745998f568e379ea197fcf827baec73ff9940", "fewtarius/llama-ai@1017f3dfdce3ca2b06aa9007b23295db3bb35722"]
  software_versions: ["SLSA 1.2", "Reproducible Builds definition accessed 2026-07-17"]
  hardware_revisions: ["dual-Strix-Halo target; exact revisions open"]
related_sections: ["73", "74", "75", "76", "77", "78", "79", "80"]
---

# Open questions

1. [OPEN] Which matrix cells are mandatory for every change, merge, nightly, periodic stress run, and release candidate?
2. [OPEN] What physical runner inventory, isolation, reset procedure, queue budget, and maximum evidence age are available?
3. [OPEN] What pilot variance and minimum practical effect establish performance thresholds for every workload/metric key?
4. [OPEN] Which statistical estimator, interval method, confirmation rule, and repetition budget will be approved?
5. [OPEN] What quality thresholds from Section 78 become hard release gates per model class?
6. [OPEN] What soak/fault durations and recovery budgets from Sections 79–80 are mandatory?
7. [OPEN] Which build environments and dependency-lock mechanism can support independent reproducibility checks?
8. [OPEN] Which signing identity, transparency log, SBOM format, and provenance level are required?
9. [OPEN] What evidence retention periods and immutable storage controls apply to change, nightly, and release artifacts?
10. [OPEN] Who may approve exceptions, what expiry is required, and which failures can never be waived?
11. [OPEN] Which cache/model/protocol format migrations require compatibility fixtures and rollback rehearsal?
12. [OPEN] What exact runtime health and client/API checks prove deployment cutover rather than only source qualification?
