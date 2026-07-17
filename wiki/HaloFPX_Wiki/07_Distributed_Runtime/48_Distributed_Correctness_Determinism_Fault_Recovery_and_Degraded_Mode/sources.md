---
section_id: "48"
title: "Distributed Correctness Sources"
status: "needs-machine-validation"
last_verified: "2026-07-16"
applies_to:
  repositories: ["ggml-org/llama.cpp@788e07dc91d266ad3162a1ce9037665656269689", "ROCm/rccl@57e58688f44c77076ad536ef1f6b68741fc6e694"]
  software_versions: ["RCCL documentation 2.27.7 (current page accessed 2026-07-16)", "PyTorch deterministic API documentation 2.9"]
  hardware_revisions: []
related_sections: ["42", "45", "53", "63"]
---

# Sources

- **S48-LLAMA-SERVER** — ggml-org, `tools/server/README.md`, commit `788e07dc91d266ad3162a1ce9037665656269689` (2026-07-17). https://github.com/ggml-org/llama.cpp/blob/788e07dc91d266ad3162a1ce9037665656269689/tools/server/README.md. Accessed 2026-07-16. Supports seed/token/slot/cache/split controls and batch-dependent nondeterminism warning. Limitation: not a distributed recovery specification.
- **S48-RCCL-SRC** — AMD ROCm, RCCL source, commit `57e58688f44c77076ad536ef1f6b68741fc6e694` (2026-01-22). https://github.com/ROCm/rccl/tree/57e58688f44c77076ad536ef1f6b68741fc6e694. Accessed 2026-07-16. Supports exact collective implementation snapshot and group/fault-injection source availability. Limitation: actual HaloFPX build/version is open.
- **S48-RCCL-API** — AMD ROCm, RCCL 2.27.7 API/error-checking documentation. https://rocm.docs.amd.com/projects/rccl/en/latest/. Accessed 2026-07-16. Supports communicator asynchronous-error API and collective semantics. Limitation: rolling `latest`; implementation claims are pinned separately to S48-RCCL-SRC.
- **S48-RCCL-BUILD** — AMD ROCm, “Building and installing RCCL from source code,” RCCL 2.27.7 page. https://rocm.docs.amd.com/projects/rccl/en/latest/install/building-installing.html. Accessed 2026-07-16. Supports build-time fault-injection option. Limitation: availability does not prove recovery.
- **S48-TORCH-DET** — PyTorch, `torch.use_deterministic_algorithms`, version 2.9 documentation. https://docs.pytorch.org/docs/2.9/generated/torch.use_deterministic_algorithms.html. Accessed 2026-07-16. Supports scoped deterministic/fail-on-unsupported precedent. Limitation: different framework/backends.
- **S48-CACHY** — fewtarius/CachyLLama, commit `6be745998f568e379ea197fcf827baec73ff9940` (2026-07-08). https://github.com/fewtarius/CachyLLama/tree/6be745998f568e379ea197fcf827baec73ff9940. Accessed 2026-07-16. Establishes cache/checkpoint reference snapshot. Limitation: its distributed crash/replay semantics are not verified here.

No target-machine evidence was produced.
