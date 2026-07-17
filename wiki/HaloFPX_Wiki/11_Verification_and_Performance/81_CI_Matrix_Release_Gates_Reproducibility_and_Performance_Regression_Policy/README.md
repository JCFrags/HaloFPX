---
section_id: "81"
title: "CI Matrix, Release Gates, Reproducibility, and Performance Regression Policy"
status: needs-machine-validation
last_verified: 2026-07-17
applies_to:
  repositories:
    - "ggml-org/llama.cpp@788e07dc91d266ad3162a1ce9037665656269689"
    - "charlie12345/ROCmFPX@a5605a72768c6562241b248e268e33dc92787394"
    - "fewtarius/CachyLLama@6be745998f568e379ea197fcf827baec73ff9940"
    - "fewtarius/llama-ai@1017f3dfdce3ca2b06aa9007b23295db3bb35722"
  software_versions: ["SLSA 1.2", "Reproducible Builds definition accessed 2026-07-17"]
  hardware_revisions: ["dual-Strix-Halo target; exact revisions open"]
related_sections: ["73", "74", "75", "76", "77", "78", "79", "80"]
---

# CI Matrix, Release Gates, Reproducibility, and Performance Regression Policy

This section turns Sections 73–80 into promotion gates. Fast CI protects basic invariants; nightly physical-hardware CI detects backend, topology, quality, and performance regressions; release-candidate qualification adds reproducibility, soak, fault, migration, security, provenance, and rollback evidence.

No HaloFPX CI run or release candidate was evaluated in this research pass. All schedules, time budgets, and statistical policies here are proposals until runner capacity and baseline variance are measured.

A green source test does not update a deployed listener. Release records must separate source/build qualification from runtime cutover, then prove the exact deployed artifact and provide rollback evidence.

See [procedures_and_checks.md](procedures_and_checks.md) for the proposed gates and [open_questions.md](open_questions.md) for thresholds and retention decisions.
