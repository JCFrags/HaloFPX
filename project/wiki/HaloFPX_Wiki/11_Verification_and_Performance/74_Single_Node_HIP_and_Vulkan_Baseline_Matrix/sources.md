---
section_id: "74"
title: "Single-node baseline sources"
status: "needs-machine-validation"
last_verified: "2026-07-16"
applies_to:
  repositories:
    - "charlie12345/ROCmFPX@a5605a72768c6562241b248e268e33dc92787394"
    - "ggml-org/llama.cpp@788e07dc91d266ad3162a1ce9037665656269689"
  software_versions: ["Linux hwmon ABI documentation 7.2"]
  hardware_revisions: []
related_sections: ["18", "22", "23", "29", "30", "33", "36", "73"]
---

# Sources

Primary code/docs were inspected at immutable commits where available. Access date: 2026-07-16 PDT. None is a HaloFPX machine measurement.

## S74-01

- Title/publisher: llama.cpp llama-bench documentation, ggml-org.
- URL: https://github.com/ggml-org/llama.cpp/blob/788e07dc91d266ad3162a1ce9037665656269689/tools/llama-bench/README.md
- Revision: commit 788e07dc91d266ad3162a1ce9037665656269689.
- Supports: pp/tg/pg, repetitions, depth/batch/KV/FA/device flags, JSONL samples, exclusion of tokenization/sampling.
- Limitation: examples are other hardware; pinned binary help remains execution authority.

## S74-02

- Title/publisher: llama.cpp server documentation and argument source, ggml-org.
- URLs: https://github.com/ggml-org/llama.cpp/blob/788e07dc91d266ad3162a1ce9037665656269689/tools/server/README.md and https://github.com/ggml-org/llama.cpp/blob/788e07dc91d266ad3162a1ce9037665656269689/common/arg.cpp
- Revision: commit 788e07dc91d266ad3162a1ce9037665656269689.
- Supports: response timings, metrics, device/KV/FA controls, speculative/MTP modes.
- Limitation: server time is not client event time; flags may be constrained.

## S74-03

- Title/publisher: llama.cpp perplexity documentation, ggml-org.
- URL: https://github.com/ggml-org/llama.cpp/blob/788e07dc91d266ad3162a1ce9037665656269689/tools/perplexity/README.md
- Revision: commit 788e07dc91d266ad3162a1ce9037665656269689.
- Supports: Wikitext-2 convention, quant/reference comparisons, implementation boundary.
- Limitation: not a complete instruction, tool, multimodal, or state quality test.

## S74-04

- Title/publisher: ROCmFPX Strix Halo ROCmFP4 + MTP build script.
- URL: https://github.com/charlie12345/ROCmFPX/blob/a5605a72768c6562241b248e268e33dc92787394/scripts/build-strix-rocmfp4-mtp.sh
- Revision: commit a5605a72768c6562241b248e268e33dc92787394; Section 16 records timestamp 2026-07-16T22:34:40-04:00.
- Supports: combined HIP/Vulkan release build, gfx1151, bench/server/perplexity/test targets.
- Limitation: build/performance unverified locally; default rocWMMA path is maintainer-local.

## S74-05

- Title/publisher: llama.cpp architecture registry.
- URL: https://github.com/ggml-org/llama.cpp/blob/788e07dc91d266ad3162a1ce9037665656269689/src/llama-arch.h
- Revision: commit 788e07dc91d266ad3162a1ce9037665656269689.
- Supports: architecture identifiers used for family coverage.
- Limitation: recognition is not end-to-end support.

## S74-06

- Title/publisher: ROCmFPX cache arguments and type registry.
- URLs: https://github.com/charlie12345/ROCmFPX/blob/a5605a72768c6562241b248e268e33dc92787394/common/arg.cpp and https://github.com/charlie12345/ROCmFPX/blob/a5605a72768c6562241b248e268e33dc92787394/ggml/include/ggml.h
- Revision: commit a5605a72768c6562241b248e268e33dc92787394.
- Supports: fork-specific K/V names and types.
- Limitation: parsing is not backend/shape/quality validation.

## S74-07

- Title/publisher: llama.cpp speculative documentation/implementation.
- URLs: https://github.com/ggml-org/llama.cpp/blob/788e07dc91d266ad3162a1ce9037665656269689/docs/speculative.md and https://github.com/ggml-org/llama.cpp/blob/788e07dc91d266ad3162a1ce9037665656269689/common/speculative.cpp
- Revision: commit 788e07dc91d266ad3162a1ce9037665656269689.
- Supports: external draft versus MTP and controller/counters.
- Limitation: no proof a selected MTP artifact fits or accelerates either backend.

## S74-08

- Title/publisher: amdgpu thermal/power hwmon interface, Linux kernel.
- URL: https://docs.kernel.org/7.2/gpu/amdgpu/thermal.html
- Revision: Linux 7.2 docs, accessed 2026-07-16.
- Supports: capability-dependent sensors and APU SoC power including CPU.
- Limitation: live presence/labels/accuracy require inspection.

## S74-09

- Title/publisher: AMD SMI documentation, AMD.
- URL: https://rocm.docs.amd.com/projects/amdsmi/en/latest/
- Revision: rolling docs, accessed 2026-07-16.
- Supports: clock, power, temperature, utilization, version query classes.
- Limitation: rolling URL; actual package and supported calls must be captured.

## Internal evidence routes

- Sections 18, 22, 23: historical inventory, power/thermal definitions, compatibility.
- Sections 29, 30, 31, 33, 36: model, quant, quality, K/V/FA, and MTP authorities.
- Section 73: proposed methodology/schema authority; implementation and machine validation remain required.
- Agent Harness at C:\Users\britt\Documents\Agent_Harness: evidence/promotion process authority, not performance evidence.
