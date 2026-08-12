---
section_id: "46"
title: "Scheduler Sources"
status: "needs-machine-validation"
last_verified: "2026-07-16"
applies_to:
  repositories: ["ggml-org/llama.cpp@788e07dc91d266ad3162a1ce9037665656269689", "vllm-project/vllm@9354f222042986addf20709e5274fc26e0d09745"]
  software_versions: []
  hardware_revisions: []
related_sections: ["39", "45", "48"]
---

# Sources

## Primary source records

- **S46-LLAMA-DEV** — ggml-org, `tools/server/README-dev.md`, commit `788e07dc91d266ad3162a1ce9037665656269689` (commit timestamp 2026-07-17). URL: https://github.com/ggml-org/llama.cpp/blob/788e07dc91d266ad3162a1ce9037665656269689/tools/server/README-dev.md. Accessed 2026-07-16. Supports slot, queue, shared-batch, compatibility, inference-thread, cancellation, and replay-buffer claims. Limitation: upstream design documentation, not HaloFPX behavior.
- **S46-LLAMA-SERVER** — ggml-org, `tools/server/README.md`, same commit/date. URL: https://github.com/ggml-org/llama.cpp/blob/788e07dc91d266ad3162a1ce9037665656269689/tools/server/README.md. Accessed 2026-07-16. Supports continuous batching, slots, seed, cache nondeterminism warning, metrics, overload status, and streaming controls. Limitation: options vary by backend.
- **S46-LLAMA-MGPU** — ggml-org, `docs/multi-gpu.md`, same commit/date. URL: https://github.com/ggml-org/llama.cpp/blob/788e07dc91d266ad3162a1ce9037665656269689/docs/multi-gpu.md. Accessed 2026-07-16. Supports split-mode semantics and upstream maturity caveats. Limitation: explicitly gives no non-NVIDIA performance guarantee for tensor mode.
- **S46-VLLM-OPT** — vLLM Project, `docs/configuration/optimization.md`, commit `9354f222042986addf20709e5274fc26e0d09745` (commit timestamp 2026-07-17). URL: https://github.com/vllm-project/vllm/blob/9354f222042986addf20709e5274fc26e0d09745/docs/configuration/optimization.md. Accessed 2026-07-16. Supports V1 chunked-prefill and preemption precedents. Limitation: different runtime and principally different accelerator deployments.
- **S46-ROCMFPX-BASE** — charlie12345/ROCmFPX, commit `a5605a72768c6562241b248e268e33dc92787394` (2026-07-16). URL: https://github.com/charlie12345/ROCmFPX/tree/a5605a72768c6562241b248e268e33dc92787394. Accessed 2026-07-16. Establishes researched fork snapshot only; scheduler behavior must be audited on the frozen integration baseline.

No target-machine evidence was produced.
