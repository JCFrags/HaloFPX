---
section_id: "06"
title: "Charter Sources"
status: "verified"
last_verified: "2026-07-16"
applies_to:
  repositories: ["charlie12345/ROCmFPX", "fewtarius/CachyLLama", "fewtarius/llama-ai", "ggml-org/llama.cpp", "local Agent_Harness"]
  software_versions: ["commits below"]
  hardware_revisions: []
related_sections: ["11", "14", "16"]
---

# Sources

## S06-01 — ROCmFPX repository

- Publisher: `charlie12345/ROCmFPX`; URL: https://github.com/charlie12345/ROCmFPX/tree/a5605a72768c6562241b248e268e33dc92787394
- Revision/date/access: commit `a5605a72768c6562241b248e268e33dc92787394`, 2026-07-17 UTC; accessed 2026-07-16 America/Los_Angeles.
- Supports: experimental status, AMD formats/backends, Strix Halo-local results, build entry points.
- Limitations: fork claims and environment-specific measurements; no dual-node HaloFPX proof.

## S06-02 — llama.cpp server documentation

- Publisher: `ggml-org/llama.cpp`; URL: https://github.com/ggml-org/llama.cpp/blob/788e07dc91d266ad3162a1ce9037665656269689/tools/server/README.md
- Revision/date/access: commit `788e07dc91d266ad3162a1ce9037665656269689`, 2026-07-17 UTC; accessed 2026-07-16.
- Supports: APIs, concurrency, caching, health, metrics, speculative serving.
- Limitations: documents upstream server, not combined or distributed behavior.

## S06-03 — CachyLLama repository

- Publisher: `fewtarius/CachyLLama`; URL: https://github.com/fewtarius/CachyLLama/tree/6be745998f568e379ea197fcf827baec73ff9940
- Revision/date/access: commit `6be745998f568e379ea197fcf827baec73ff9940`, 2026-07-09; accessed 2026-07-16.
- Supports: SSD cache, user isolation, expert telemetry, declared benchmark context.
- Limitations: fork documentation and specific-machine results; integrity and compatibility require audit.

## S06-04 — llama-ai repository

- Publisher: `fewtarius/llama-ai`; URL: https://github.com/fewtarius/llama-ai/tree/1017f3dfdce3ca2b06aa9007b23295db3bb35722
- Revision/date/access: commit `1017f3dfdce3ca2b06aa9007b23295db3bb35722`, 2026-07-09; accessed 2026-07-16.
- Supports: offline AMD APU goal, runner/tooling context, workload descriptions.
- Limitations: project-specific claims; model names and results require independent verification.

## S06-05 — Agent Harness architecture

- Publisher/path: local project authority, `C:\Users\britt\Documents\Agent_Harness\guide\architecture.md`.
- Revision/access: working-tree document accessed 2026-07-16; no immutable commit supplied.
- Supports: sources-to-wiki routing, evidence promotion, memory scope, review discipline.
- Limitations: local conceptual authority, not external proof of HaloFPX behavior.

