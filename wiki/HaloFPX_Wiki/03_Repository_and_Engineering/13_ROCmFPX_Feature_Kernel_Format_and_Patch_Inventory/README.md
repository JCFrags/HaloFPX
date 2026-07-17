---
section_id: "13"
title: "ROCmFPX Feature, Kernel, Format, and Patch Inventory"
status: "needs-machine-validation"
last_verified: "2026-07-17"
applies_to:
  repositories:
    - "charlie12345/ROCmFPX@a5605a72768c6562241b248e268e33dc92787394"
    - "ggml-org/llama.cpp@788e07dc91d266ad3162a1ce9037665656269689"
  software_versions: []
  hardware_revisions:
    - "AMD Strix Halo / gfx1151 (project target; not measured in this section)"
related_sections: ["11", "12", "15", "16", "23", "30", "31", "33", "36", "37", "66", "73", "74", "78"]
---

# ROCmFPX feature, kernel, format, and patch inventory

<a id="s13-status"></a>
## Executive status

**[VERIFIED]** This inventory is pinned to ROCmFPX `main` commit `a5605a72768c6562241b248e268e33dc92787394` (2026-07-16) and contemporaneous llama.cpp `master` `788e07dc91d266ad3162a1ce9037665656269689`. The fork contains real custom GGUF weight types, CPU references, HIP/ROCm and Vulkan integration, model conversion/quantization tooling, native-MTP adaptations, TurboQuant K/V-cache types, server wrappers, and a large regression-script surface [S13-01, S13-02, S13-03, S13-04, S13-05, S13-06, S13-07, S13-08, S13-09].

**[VERIFIED]** The current code, not older handoff prose, is authoritative for the format list. It registers `Q2_0_ROCMFPX`, `Q3_0_ROCMFPX`, `Q4_0_ROCMFP4`, `Q4_0_ROCMFP4_FAST`, `Q6_0_ROCMFPX`, and `Q8_0_ROCMFPX`; `turbo3` and `turbo4` are runtime cache types, not model-weight formats [S13-02, S13-03, S13-04, S13-05, S13-06].

**[OPEN]** No code was built and no model was run on either HaloFPX machine for this section. Repository benchmark tables and “passed” statements are upstream/fork-reported results, not HaloFPX **[MEASURED]** evidence.

**[MEASURED]** The live cluster currently runs the separate predecessor `charlie12345/rocmfp4-llama@4860505ee322091f0f61eba77d6ad49be88cf4ea`, not the pinned ROCmFPX goal tree [S13-L01]. Its executable hashes, process roles, model placement, and MPTCP topology are now preserved as a comparison baseline; they do not validate ROCmFPX `a5605a7`.

<a id="s13-map"></a>
## Retrieval map

| Need | Page |
|---|---|
| Formats, presets, kernels, tests, wrappers, patch inventory | [Facts and constraints](facts_and_constraints.md) |
| HaloFPX choices and upstream-conflict implications | [Design implications](design_implications.md) |
| Reproducible source and two-machine checks | [Procedures and checks](procedures_and_checks.md) |
| Unresolved decisions and evidence gaps | [Open questions](open_questions.md) |
| Stable primary-source records | [Sources](sources.md) |

<a id="s13-boundary"></a>
## Boundary

This section inventories what exists and how mature it appears. Section 30 owns quantization recipes, section 33 owns attention/KV-quality decisions, section 36 owns speculative-decoding policy, section 37 owns gfx1151 optimization, section 15 owns the integration stack, and sections 73/74/78 own measurements and release gates.

<a id="s13-research-split"></a>
## Research split

1. **Internet/source-code research completed:** exact heads, layouts, presets, backend wiring, conversion, capability detection, wrappers, tests, attribution, and high-conflict surfaces.
2. **Machine work required:** clean builds on both nodes; reference/backend-op tests; matched model conversion; CPU/HIP/Vulkan parity; MTP and TurboQuant quality; server/cache failure tests.
3. **Contingent decisions:** admissible formats, default backend, cache types, MTP profiles, and which fork patches to carry.
