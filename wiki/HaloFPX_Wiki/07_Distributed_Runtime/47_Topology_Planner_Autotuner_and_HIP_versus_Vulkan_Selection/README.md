---
section_id: "47"
title: "Topology Planner, Autotuner, and HIP-versus-Vulkan Selection"
status: "needs-machine-validation"
last_verified: "2026-07-16"
applies_to:
  repositories: ["ggml-org/llama.cpp@788e07dc91d266ad3162a1ce9037665656269689", "charlie12345/ROCmFPX@a5605a72768c6562241b248e268e33dc92787394"]
  software_versions: []
  hardware_revisions: ["two matched AMD Strix Halo systems (exact BOM open)"]
related_sections: ["17", "23", "24", "25", "37", "38", "42", "43", "44", "67", "74", "76"]
---

# Topology planner, autotuner, and backend selection

**[VERIFIED]** llama.cpp exposes `none`, `layer`, `row`, and experimental `tensor` split modes, explicit tensor splits, main device, fit targets, and separately compiled HIP and Vulkan backends [S47-LLAMA-SERVER, S47-LLAMA-MGPU, S47-LLAMA-CMAKE]. **[VERIFIED]** Its current multi-GPU guide characterizes layer mode as pipeline-oriented and tensor mode as collective-heavy, while declining non-NVIDIA performance guarantees [S47-LLAMA-MGPU]. Therefore no primary source proves the best Strix Halo/USB4 plan.

**[RECOMMENDATION]** Treat planning as reproducible constrained optimization, not startup guesswork. The output is a signed/hashable plan manifest tied to exact model, runtime, backend, hardware, firmware, driver, transport, cache, and objective fingerprints. Promote a candidate only after correctness gates, repeated measurements, confidence checks, and canary rollback.

## Research split

- **Internet/source-code research completed:** pinned available upstream controls, backend build flags, and benchmark interfaces.
- **On-machine work required:** measure HIP and Vulkan for every supported operator/model shape; fabric and collective curves; memory headroom; power/thermal behavior; plan stability under concurrency and cache states.
- **Contingent decisions:** backend, execution mode, split point, collective policy, cache layout, MTP/speculation, batch limits, and power profile.

No **[MEASURED]** target-machine result exists. See [facts](facts_and_constraints.md), [design](design_implications.md), [procedure](procedures_and_checks.md), [questions](open_questions.md), and [sources](sources.md).
