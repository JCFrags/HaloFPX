---
section_id: "68"
title: "Lifecycle and Routing Sources"
status: "verified"
last_verified: "2026-07-16"
applies_to:
  repositories: ["ggml-org/llama.cpp", "fewtarius/CachyLLama", "charlie12345/ROCmFPX", "local Agent_Harness"]
  software_versions: ["commits below"]
  hardware_revisions: []
related_sections: ["38", "39", "45", "46", "48", "60", "61"]
---

# Sources

## S68-01 — llama.cpp server documentation

- URL/revision: https://github.com/ggml-org/llama.cpp/blob/788e07dc91d266ad3162a1ce9037665656269689/tools/server/README.md; commit `788e07dc91d266ad3162a1ce9037665656269689`, 2026-07-17 UTC; accessed 2026-07-16.
- Supports: router mode, model sources/layout, dynamic load/unload, warmup, slots, continuous batching, cache, health, and sleep-on-idle.
- Limitations: upstream single-process/router documentation; not HaloFPX rank/session semantics.

## S68-02 — CachyLLama README and user isolation design pointer

- URL/revision: https://github.com/fewtarius/CachyLLama/blob/6be745998f568e379ea197fcf827baec73ff9940/README.md; commit `6be745998f568e379ea197fcf827baec73ff9940`, 2026-07-09; accessed 2026-07-16.
- Supports: user-scoped checkpoints, per-user 429 limits, slot affinity, persistent cache controls.
- Limitations: fork-specific claims; exact code/fault behavior needs audit and reproduction.

## S68-03 — ROCmFPX repository

- URL/revision: https://github.com/charlie12345/ROCmFPX/tree/a5605a72768c6562241b248e268e33dc92787394; commit `a5605a72768c6562241b248e268e33dc92787394`, 2026-07-17 UTC; accessed 2026-07-16.
- Supports: intended AMD model runtime, build paths, and backend/model-specific constraints.
- Limitations: no reviewed HaloFPX distributed lifecycle implementation.

## S68-04 — llama-ai repository

- URL/revision: https://github.com/fewtarius/llama-ai/tree/1017f3dfdce3ca2b06aa9007b23295db3bb35722; commit `1017f3dfdce3ca2b06aa9007b23295db3bb35722`, 2026-07-09; accessed 2026-07-16.
- Supports: end-to-end AMD APU runner and workload context around CachyLLama.
- Limitations: deployment-specific scripts and measurements; not a product lifecycle standard.

## S68-05 — Agent Harness architecture

- Path/revision: `C:\Users\britt\Documents\Agent_Harness\guide\architecture.md`; local working-tree authority accessed 2026-07-16.
- Supports: lifecycle state, provenance, validation, review, and rollback discipline.
- Limitations: conceptual/local and not executable lifecycle code.

