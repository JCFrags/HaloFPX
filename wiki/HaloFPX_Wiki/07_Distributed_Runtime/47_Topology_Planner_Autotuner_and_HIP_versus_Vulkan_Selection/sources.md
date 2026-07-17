---
section_id: "47"
title: "Planner Sources"
status: "needs-machine-validation"
last_verified: "2026-07-16"
applies_to:
  repositories: ["ggml-org/llama.cpp@788e07dc91d266ad3162a1ce9037665656269689", "charlie12345/ROCmFPX@a5605a72768c6562241b248e268e33dc92787394", "fewtarius/CachyLLama@6be745998f568e379ea197fcf827baec73ff9940"]
  software_versions: []
  hardware_revisions: []
related_sections: ["37", "38", "74", "76"]
---

# Sources

- **S47-LLAMA-SERVER** — ggml-org, `tools/server/README.md`, commit `788e07dc91d266ad3162a1ce9037665656269689` (2026-07-17). https://github.com/ggml-org/llama.cpp/blob/788e07dc91d266ad3162a1ce9037665656269689/tools/server/README.md. Accessed 2026-07-16. Supports concrete planning controls. Limitation: not Strix Halo evidence.
- **S47-LLAMA-MGPU** — ggml-org, `docs/multi-gpu.md`, same commit/date. https://github.com/ggml-org/llama.cpp/blob/788e07dc91d266ad3162a1ce9037665656269689/docs/multi-gpu.md. Accessed 2026-07-16. Supports mode semantics and maturity caveats. Limitation: multi-GPU, not custom inter-host USB4.
- **S47-LLAMA-CMAKE** — ggml-org, `ggml/CMakeLists.txt`, same commit/date. https://github.com/ggml-org/llama.cpp/blob/788e07dc91d266ad3162a1ce9037665656269689/ggml/CMakeLists.txt. Accessed 2026-07-16. Supports HIP, RCCL, graph, Vulkan, and validation build options. Limitation: build availability does not prove runtime support.
- **S47-LLAMA-BENCH** — ggml-org, `tools/llama-bench/README.md`, same commit/date. https://github.com/ggml-org/llama.cpp/blob/788e07dc91d266ad3162a1ce9037665656269689/tools/llama-bench/README.md. Accessed 2026-07-16. Supports benchmark controls. Limitation: microbenchmark only.
- **S47-ROCMFPX** — charlie12345/ROCmFPX, commit `a5605a72768c6562241b248e268e33dc92787394` (2026-07-16). https://github.com/charlie12345/ROCmFPX/tree/a5605a72768c6562241b248e268e33dc92787394. Accessed 2026-07-16. Establishes fork snapshot; exact integrated patch stack remains open.
- **S47-CACHY** — fewtarius/CachyLLama, commit `6be745998f568e379ea197fcf827baec73ff9940` (2026-07-08). https://github.com/fewtarius/CachyLLama/tree/6be745998f568e379ea197fcf827baec73ff9940. Accessed 2026-07-16. Establishes cache/server reference snapshot. Limitation: not a validated planner.

No target-machine measurements were used.
