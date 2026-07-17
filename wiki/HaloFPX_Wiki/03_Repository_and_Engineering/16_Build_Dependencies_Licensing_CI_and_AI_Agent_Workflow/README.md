---
section_id: "16"
title: "Build, Dependencies, Licensing, CI, and AI-Agent Workflow"
status: "needs-machine-validation"
last_verified: "2026-07-16"
applies_to:
  repositories:
    - "charlie12345/ROCmFPX@a5605a72768c6562241b248e268e33dc92787394"
    - "ggml-org/llama.cpp@788e07dc91d266ad3162a1ce9037665656269689"
    - "fewtarius/CachyLLama@6be745998f568e379ea197fcf827baec73ff9940"
    - "fewtarius/llama-ai@1017f3dfdce3ca2b06aa9007b23295db3bb35722"
  software_versions:
    - "CMake 3.14...3.28 project policy range"
    - "C 11 / C++ 17"
  hardware_revisions:
    - "dual Strix Halo / gfx1151 (planned; not measured here)"
related_sections: ["03.11", "03.13", "03.14", "03.15", "04", "11"]
---

# Section 16: Engineering control plane

**[VERIFIED]** The intended source repositories are not equivalent engineering inputs. ROCmFPX, llama.cpp, and CachyLLama are MIT-licensed CMake projects with broad inherited CI and the same upstream AI-contribution restrictions at the pinned revisions. `llama-ai` is a GPL-3.0-or-later shell/orchestration repository that pins CachyLLama as a Git submodule. [S16-01][S16-05][S16-07][S16-09]

**[RECOMMENDATION]** HaloFPX should preserve an MIT source core derived from ROCmFPX, import CachyLLama behavior only through reviewed commits or newly authored changes with provenance, and keep GPL-covered `llama-ai` scripts outside that core unless the project deliberately accepts the resulting distribution obligations after legal review. This is engineering guidance, not legal advice.

**[RECOMMENDATION]** A change is mergeable only when all four records agree:

1. exact source and dependency revisions;
2. a clean, reproducible build manifest;
3. required tests and matched-performance evidence;
4. a human-owned review record plus append-only AI-assistance entry when applicable.

## Retrieval map

| Need | Page |
|---|---|
| Pinned facts, backend/build matrix, licenses, CI inventory | [facts_and_constraints.md](facts_and_constraints.md) |
| Project policy and gate design | [design_implications.md](design_implications.md) |
| Build capture, CI gates, AI log schema, release checks | [procedures_and_checks.md](procedures_and_checks.md) |
| Unresolved dependencies and machine work | [open_questions.md](open_questions.md) |
| Stable primary-source records | [sources.md](sources.md) |

## Research split

### Completed from source and Internet research

- exact HEAD pins and `llama-ai` submodule pin;
- CMake language standards, backend switches, generated build-info path, dependency mechanisms, licenses/notices, contribution policy, and CI definitions;
- SPDX 3.0.1, SLSA 1.2 provenance, GPL compatibility guidance, and `SOURCE_DATE_EPOCH` reference material. [S16-12][S16-13][S16-14][S16-15]

### Required on both Strix Halo machines

- record the real OS, kernel, firmware, ROCm/LLVM, Vulkan loader/driver, CMake, Ninja, Python, and compiler identities;
- perform clean HIP+Vulkan builds from the exact baseline with network access disabled after dependency materialization;
- run CPU reference, HIP, Vulkan, cache-corruption, distributed fallback, and matched performance gates;
- rebuild twice in isolated paths and compare artifacts after accounting for documented non-determinism.

### Decisions contingent on those results

- the supported ROCm and compiler baseline;
- whether one combined HIP+Vulkan binary or separate artifacts are shipped;
- which self-hosted GPU jobs are mandatory versus scheduled;
- whether bit-for-bit output is attainable or whether normalized reproducibility is the release contract;
- whether any GPL `llama-ai` code may enter a distributed HaloFPX artifact.

No build, test, benchmark, or legal review was performed by this research section.

