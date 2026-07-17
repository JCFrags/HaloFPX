---
section_id: "10"
title: "Architecture Principle Sources"
status: "verified"
last_verified: "2026-07-16"
applies_to:
  repositories: ["charlie12345/ROCmFPX", "ggml-org/llama.cpp", "fewtarius/CachyLLama", "fewtarius/llama-ai", "local Agent_Harness"]
  software_versions: ["commits below"]
  hardware_revisions: []
related_sections: ["15", "38", "47", "48"]
---

# Sources

## S10-01 — ROCmFPX README and source tree

- URL/revision: https://github.com/charlie12345/ROCmFPX/tree/a5605a72768c6562241b248e268e33dc92787394; `a5605a72768c6562241b248e268e33dc92787394`, 2026-07-17 UTC; accessed 2026-07-16.
- Supports: experimental/qualified status, backend/model/workload-specific results, reference paths.
- Limitations: local project evidence, not a universal architecture rule or two-node result.

## S10-02 — llama.cpp server README

- URL/revision: https://github.com/ggml-org/llama.cpp/blob/788e07dc91d266ad3162a1ce9037665656269689/tools/server/README.md; `788e07dc91d266ad3162a1ce9037665656269689`, 2026-07-17 UTC; accessed 2026-07-16.
- Supports: existing server mechanisms, timings, metrics, prompt-cache behavior and nondeterminism caveat.
- Limitations: fast-moving upstream and no HaloFPX distributed semantics.

## S10-03 — CachyLLama README

- URL/revision: https://github.com/fewtarius/CachyLLama/blob/6be745998f568e379ea197fcf827baec73ff9940/README.md; `6be745998f568e379ea197fcf827baec73ff9940`, 2026-07-09; accessed 2026-07-16.
- Supports: optional cache/telemetry features and scoped benchmark claims.
- Limitations: source and fault audit outstanding; no general distributed result.

## S10-04 — Agent Harness architecture

- Path/revision: `C:\Users\britt\Documents\Agent_Harness\guide\architecture.md`; local working-tree authority accessed 2026-07-16.
- Supports: evidence promotion, deterministic/semantic work split, validation, rollback, and review.
- Limitations: conceptual/local and not immutable.

## S10-05 — llama-ai README

- URL/revision: https://github.com/fewtarius/llama-ai/blob/1017f3dfdce3ca2b06aa9007b23295db3bb35722/README.md; `1017f3dfdce3ca2b06aa9007b23295db3bb35722`, 2026-07-09; accessed 2026-07-16.
- Supports: offline AMD APU workload and measured-cache context.
- Limitations: project-specific evidence; not representative until HaloFPX traces are captured.

