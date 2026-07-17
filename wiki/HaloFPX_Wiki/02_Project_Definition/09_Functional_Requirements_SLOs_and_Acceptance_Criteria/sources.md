---
section_id: "09"
title: "Requirements and SLO Sources"
status: "verified"
last_verified: "2026-07-16"
applies_to:
  repositories: ["ggml-org/llama.cpp", "fewtarius/CachyLLama", "charlie12345/ROCmFPX", "fewtarius/llama-ai", "local Agent_Harness"]
  software_versions: ["commits below"]
  hardware_revisions: []
related_sections: ["68", "69", "74", "78"]
---

# Sources

## S09-01 — llama.cpp server README

- URL/revision: https://github.com/ggml-org/llama.cpp/blob/788e07dc91d266ad3162a1ce9037665656269689/tools/server/README.md; `788e07dc91d266ad3162a1ce9037665656269689`, 2026-07-17 UTC; accessed 2026-07-16.
- Supports: endpoint behavior, health 200/503 semantics, API keys/TLS inputs, streaming, timings, metrics, slots/cache and concurrency controls.
- Limitations: upstream explicitly qualifies API compatibility; not distributed acceptance evidence.

## S09-02 — CachyLLama README

- URL/revision: https://github.com/fewtarius/CachyLLama/blob/6be745998f568e379ea197fcf827baec73ff9940/README.md; `6be745998f568e379ea197fcf827baec73ff9940`, 2026-07-09; accessed 2026-07-16.
- Supports: cache flags, user IDs/concurrency, expert telemetry, reported local timings.
- Limitations: reported results are scoped; correctness/security details require source and machine tests.

## S09-03 — ROCmFPX README

- URL/revision: https://github.com/charlie12345/ROCmFPX/blob/a5605a72768c6562241b248e268e33dc92787394/README.md; `a5605a72768c6562241b248e268e33dc92787394`, 2026-07-17 UTC; accessed 2026-07-16.
- Supports: experimental status, AMD backend/format options, environment-qualified benchmark format.
- Limitations: not a two-node performance or quality baseline.

## S09-04 — llama-ai README

- URL/revision: https://github.com/fewtarius/llama-ai/blob/1017f3dfdce3ca2b06aa9007b23295db3bb35722/README.md; `1017f3dfdce3ca2b06aa9007b23295db3bb35722`, 2026-07-09; accessed 2026-07-16.
- Supports: benchmark/workload examples and offline AMD APU intent.
- Limitations: project-specific measurements; no HaloFPX acceptance authority.

## S09-05 — Agent Harness architecture

- Path/revision: `C:\Users\britt\Documents\Agent_Harness\guide\architecture.md`; local working-tree authority accessed 2026-07-16.
- Supports: evidence promotion, evaluation, review, provenance, rollback discipline.
- Limitations: conceptual/local, no immutable commit or machine result.

