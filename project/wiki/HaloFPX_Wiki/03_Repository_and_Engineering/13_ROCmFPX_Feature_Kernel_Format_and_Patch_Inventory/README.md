---
section_id: "13"
title: "ROCmFPX Feature, Kernel, Format, and Patch Inventory"
status: "needs-machine-validation"
last_verified: "2026-08-12"
applies_to:
  repositories:
    - "charlie12345/ROCmFPX@a5605a72768c6562241b248e268e33dc92787394"
    - "ggml-org/llama.cpp@788e07dc91d266ad3162a1ce9037665656269689"
    - "JCFrags/HaloFPX@4a156395db62604cf37e27e6459e3ee0e3949c48"
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

**[VERIFIED current-source reconciliation]** HaloFPX `4a156395` retains that
format list. Q2 has CPU and partial CUDA/HIP static wiring (dequantization,
`GET_ROWS`, MMVQ, and MMQ). Generic same-type contiguous device copy remains
available, but Q2 lacks conversion/noncontiguous `CPY`, `SET_ROWS`, Vulkan,
the common application K/V-cache CLI allowlist, wrapper, and agent-preset
surfaces. Q3/Q4/Q4_FAST/Q6/Q8 retain CPU, CUDA/HIP, and Vulkan paths. Q6
serializes the signed range `[-32, 31]`; its 26-byte GGUF
block expands to 34 bytes in the Vulkan device layout [S13-L04]. Static wiring
is not model compatibility, quality, or performance qualification.

**[MEASURED]** The control `a5605a7` and candidate `61f2f2d` now build reproducibly from the locked offline bundle on both target nodes. CPU references, TurboQuant 7/7, ROCmFP4 quant regression, and the ROCm0 `FLASH_ATTN_EXT` matrices pass: 2881/2881 control and 2899/2899 candidate on each node [S13-L02].

**[MEASURED]** Both commits also load the same hashed Qwen3-4B-Q8_0 model and complete deterministic loopback requests with F16 and Turbo4 cache on each node [S13-L03]. F16 output is identical across commits/nodes; Turbo4 output is likewise identical across commits/nodes but differs from F16. This is a quality gate, not a candidate-commit regression. Large-model, long-context, MTP, RPC, SSD-state, Vulkan-parity, and performance qualification remain **[OPEN]**.

The following capture-scoped claim is preserved verbatim from the 2026-07-17
inventory. In that sentence, “currently” means at the time of S13-L01; it is
not current deployment authority after the dated reconciliation below.

**[MEASURED]** The live cluster currently runs the separate predecessor `charlie12345/rocmfp4-llama@4860505ee322091f0f61eba77d6ad49be88cf4ea`, not the pinned ROCmFPX goal tree [S13-L01]. Its executable hashes, process roles, model placement, and MPTCP topology are now preserved as a comparison baseline; they do not validate ROCmFPX `a5605a7`.

**[MEASURED]** On 2026-08-12 the always-on service instead used upstream
llama.cpp with conventional UD-Q6 weights. This supersedes S13-L01 only as
current deployment authority; neither deployment validates current
HaloFPX/ROCmFPX [S13-L05].

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
