---
section_id: "07"
title: "Users and Workloads Sources"
status: "verified"
last_verified: "2026-07-16"
applies_to:
  repositories: ["ggml-org/llama.cpp", "fewtarius/CachyLLama", "fewtarius/llama-ai", "charlie12345/ROCmFPX"]
  software_versions: ["commits below"]
  hardware_revisions: []
related_sections: ["32", "60", "69"]
---

# Sources

## S07-01 — llama.cpp server README

- URL: https://github.com/ggml-org/llama.cpp/blob/788e07dc91d266ad3162a1ce9037665656269689/tools/server/README.md
- Publisher/revision: `ggml-org/llama.cpp`, commit `788e07dc91d266ad3162a1ce9037665656269689`, 2026-07-17 UTC; accessed 2026-07-16.
- Supports: API surface, streaming, concurrency, tool use, timings, cache and metrics controls.
- Limitations: compatibility is qualified by upstream; not a HaloFPX workload guarantee.

## S07-02 — CachyLLama README

- URL: https://github.com/fewtarius/CachyLLama/blob/6be745998f568e379ea197fcf827baec73ff9940/README.md
- Publisher/revision: `fewtarius/CachyLLama`, commit `6be745998f568e379ea197fcf827baec73ff9940`, 2026-07-09; accessed 2026-07-16.
- Supports: persistent cache options, user IDs/namespaces, slot affinity, concurrency limit, expert telemetry.
- Limitations: fork claims and specific-machine benchmarks require reproduction and source audit.

## S07-03 — llama-ai README

- URL: https://github.com/fewtarius/llama-ai/blob/1017f3dfdce3ca2b06aa9007b23295db3bb35722/README.md
- Publisher/revision: `fewtarius/llama-ai`, commit `1017f3dfdce3ca2b06aa9007b23295db3bb35722`, 2026-07-09; accessed 2026-07-16.
- Supports: offline AMD APU goal and described agent/prefix-reuse workload.
- Limitations: described client and measurements are not representative until compared with actual HaloFPX traces.

## S07-04 — ROCmFPX README

- URL: https://github.com/charlie12345/ROCmFPX/blob/a5605a72768c6562241b248e268e33dc92787394/README.md
- Publisher/revision: `charlie12345/ROCmFPX`, commit `a5605a72768c6562241b248e268e33dc92787394`, 2026-07-17 UTC; accessed 2026-07-16.
- Supports: AMD format choices, structured-output-oriented presets, MTP workload sensitivity.
- Limitations: project claims and local results; no general quality guarantee.

## S07-05 — Agent Harness architecture

- Path: `C:\Users\britt\Documents\Agent_Harness\guide\architecture.md`; local authority accessed 2026-07-16.
- Supports: evidence promotion, review, and scoped run experience.
- Limitations: no immutable revision; not user research.

