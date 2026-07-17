---
section_id: "74"
title: "Single-node baseline open questions"
status: "needs-machine-validation"
last_verified: "2026-07-16"
applies_to:
  repositories:
    - "charlie12345/ROCmFPX@a5605a72768c6562241b248e268e33dc92787394"
    - "ggml-org/llama.cpp@788e07dc91d266ad3162a1ce9037665656269689"
  software_versions: []
  hardware_revisions: ["nimo-1", "nimo-2"]
related_sections: ["18", "22", "23", "29", "31", "33", "36", "73", "78", "79", "81"]
---

# Open questions

| ID | Question | Resolution evidence | Blocks |
|---|---|---|---|
| OQ74-01 | **[OPEN]** What current BIOS, EC, firmware, kernel, ROCm, Mesa/RADV, power, and cooling tuple exists per node? | Fresh Section 18/22/23 raw inventories. | paired comparison |
| OQ74-02 | **[OPEN]** Which proposed Section 73 warm-up, confidence, outlier, and regression controls pass implementation and machine validation? | Implement its schema and complete its required pilot experiments. | ACCEPTED/release thresholds |
| OQ74-03 | **[OPEN]** Which exact model/tokenizer/GGUF hashes represent each family, and what quality reference fits each context? | Section 29/31 manifests/load gates. | execution |
| OQ74-04 | **[OPEN]** Which ROCmFPX weight preset is approved per dense, MoE, hybrid, multimodal, and MTP topology? | Section 30/31 tensor-map/quality evidence. | custom quant rows |
| OQ74-05 | **[OPEN]** Which K/V types are correct for each backend/model/FA tuple? | backend-op, deterministic, quality, long-context evidence. | compressed KV |
| OQ74-06 | **[OPEN]** Does FA on dispatch the intended kernel for every shape/type, and how is fallback observed? | verbose trace/profiler/operator tests. | FA comparison |
| OQ74-07 | **[OPEN]** Which device names isolate HIP and Vulkan in the combined binary? | device list plus loader/process evidence. | backend isolation |
| OQ74-08 | **[OPEN]** Which compatible, locally fitting MTP artifact is the candidate? | Pinned model/config/license/GGUF and Section 36 gate. | MTP |
| OQ74-09 | **[OPEN]** What request/token counts stabilize p95/p99 ILT? | Section 73 pilot power/sample study. | tail claims |
| OQ74-10 | **[OPEN]** Which APU telemetry domains are available/non-overlapping, and are wall meters calibrated? | sensor map, AMD SMI probe, calibration. | energy/thermal |
| OQ74-11 | **[OPEN]** What node deviation means the machines are not matched? | repeated paired controls with uncertainty. | distributed baseline |
| OQ74-12 | **[OPEN]** What memory headroom is required for server, concurrency, and transport? | allocation tests plus Sections 19/46/76/79. | max context |
| OQ74-13 | **[OPEN]** Are power modes firmware presets, package limits, wall targets, or all three? | Section 22 decision and stable controls. | power sweep |
| OQ74-14 | **[OPEN]** How are multimodal preprocessing and SSM state benchmarked without conflating text-only engine results? | Section 29/35/78 protocols. | family completeness |

## Internet/source follow-up

1. Recheck pinned documentation and implementation together; record flag/doc contradictions rather than using current master.
2. Pin immutable publisher config, license, tokenizer, model, and GGUF conversion provenance for each model.
3. Inspect each ROCmFPX preset's exact per-tensor mapping and backend operator coverage.
4. Pin distro ROCm, Mesa/RADV, kernel, firmware, AMD SMI, and Vulkan package revisions.

## On-machine follow-up

1. Run paired inventory and device-enumeration preflight.
2. Complete one dense Q4_K_M, F16-KV, FA-off anchor on both nodes/backends.
3. Prove logging captures actual backend/FA/MTP paths and client token timestamps.
4. Pilot sustained runs for warm-up/sample sizing without publishing performance.
5. Run quality/memory gates before expanding context/KV/MTP.
6. Submit evidence for independent review before MEASURED_CANDIDATE.
