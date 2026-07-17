---
section_id: "08"
title: "Scope and Boundary Sources"
status: "verified"
last_verified: "2026-07-16"
applies_to:
  repositories: ["charlie12345/ROCmFPX", "fewtarius/CachyLLama", "fewtarius/llama-ai", "ggml-org/llama.cpp", "local Agent_Harness"]
  software_versions: ["commits below"]
  hardware_revisions: []
related_sections: ["11", "13", "14", "15", "16"]
---

# Sources

## S08-01 — ROCmFPX

- URL/revision: https://github.com/charlie12345/ROCmFPX/tree/a5605a72768c6562241b248e268e33dc92787394; commit `a5605a72768c6562241b248e268e33dc92787394`, 2026-07-17 UTC; accessed 2026-07-16.
- Supports: experimental fork status, AMD formats/backends, declared tested targets.
- Limitations: no supported HaloFPX product matrix or two-node implementation.

## S08-02 — llama.cpp

- URL/revision: https://github.com/ggml-org/llama.cpp/tree/788e07dc91d266ad3162a1ce9037665656269689; commit `788e07dc91d266ad3162a1ce9037665656269689`, 2026-07-17 UTC; accessed 2026-07-16.
- Supports: upstream code and server boundary.
- Limitations: fast-moving head; downstream compatibility requires exact tests.

## S08-03 — CachyLLama

- URL/revision: https://github.com/fewtarius/CachyLLama/tree/6be745998f568e379ea197fcf827baec73ff9940; commit `6be745998f568e379ea197fcf827baec73ff9940`, 2026-07-09; accessed 2026-07-16.
- Supports: candidate cache/isolation/telemetry features and runtime flags.
- Limitations: source audit and format/fault validation outstanding.

## S08-04 — llama-ai

- URL/revision: https://github.com/fewtarius/llama-ai/tree/1017f3dfdce3ca2b06aa9007b23295db3bb35722; commit `1017f3dfdce3ca2b06aa9007b23295db3bb35722`, 2026-07-09; accessed 2026-07-16.
- Supports: parent scripts, AMD APU target, CachyLLama submodule context.
- Limitations: not an external compatibility standard.

## S08-05 — Agent Harness architecture

- Path/revision: `C:\Users\britt\Documents\Agent_Harness\guide\architecture.md`; working-tree authority accessed 2026-07-16.
- Supports: evidence routing, lifecycle, provenance, and reversible promotion.
- Limitations: no immutable commit; conceptual/local scope.

