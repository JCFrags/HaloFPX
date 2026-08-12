---
section_id: "74"
title: "Single-node baseline facts and constraints"
status: "needs-machine-validation"
last_verified: "2026-07-16"
applies_to:
  repositories:
    - "charlie12345/ROCmFPX@a5605a72768c6562241b248e268e33dc92787394"
    - "ggml-org/llama.cpp@788e07dc91d266ad3162a1ce9037665656269689"
  software_versions: []
  hardware_revisions: ["nimo-1 and nimo-2; exact current inventory required"]
related_sections: ["18", "22", "23", "29", "30", "33", "36", "73", "78", "79"]
---

# Facts and constraints

## Tool boundaries

| ID | Source-backed fact | Constraint for Section 74 |
|---|---|---|
| 74-F01 | **[VERIFIED]** Pinned llama-bench supports pp, tg, pg, repetitions, JSONL, context depth, batch/ubatch, K/V type, FA, and device [S74-01]. | Use it for engine-only throughput, not client TTFT or ILT percentiles. |
| 74-F02 | **[VERIFIED]** llama-bench excludes tokenization and sampling [S74-01]. | Label rates engine_pp_tps/engine_tg_tps; do not relabel end-to-end. |
| 74-F03 | **[VERIFIED]** Server responses include prompt/prediction timing; optional /metrics exports aggregate counters/gauges [S74-02]. | A streaming client with monotonic timestamps is required for per-request TTFT and p50/p95/p99 ILT. |
| 74-F04 | **[VERIFIED]** Upstream accepts K/V types f32, f16, bf16, q8_0, q4_0, q4_1, iq4_nl, q5_0, q5_1; ROCmFPX adds custom types including Turbo3/4 [S74-02][S74-06]. | Every backend/type/model tuple needs smoke and quality gates. |
| 74-F05 | **[VERIFIED]** Speculation includes none, external draft modes, and draft-mtp, with draft artifact/device/depth controls [S74-02][S74-07]. | MTP=on requires a pinned compatible artifact; otherwise record NOT_APPLICABLE. |
| 74-F06 | **[VERIFIED]** The pinned ROCmFPX Strix script builds HIP and Vulkan in one release tree, targets gfx1151, and builds bench/server/perplexity/tests [S74-04]. | One binary reduces drift, but each backend is independently qualified. |
| 74-F07 | **[VERIFIED]** Upstream recommends Wikitext-2 perplexity for quant comparisons and warns results depend on implementation details [S74-03]. | Compare only same model/tokenizer/corpus/runtime and add Section 31/78 checks. |

## Machine boundary

**[OPEN]** Section 18 contains a historical description of both nodes, but it is not current Section 74 preflight evidence and no field is carried forward as a benchmark control.

**[OPEN]** BIOS/EC, kernel, firmware, ROCm, HIP, Mesa/RADV, model placement, power controls, cooling, ambient, and sensor support must be recaptured for each run.

## Canonical trial fields

| Class | Required fields |
|---|---|
| identity | run/trial UUID, UTC and monotonic times, hostname, inventory digest, full commit/tree/dirty state, binary/build hashes |
| artifact | family/architecture, publisher revision, model/tokenizer/GGUF hashes, weight quant/preset, draft/MTP hashes |
| execution | exact device/backend, threads/affinity, context/depth, prompt token count/hash, output target, batch/ubatch, K/V, FA and MTP requested/observed, seed/sampler, cache state |
| environment | OS/kernel/cmdline, amdgpu/firmware, ROCm/HIP, loader/Mesa/RADV, governor/EPP, named power mode/limits, ambient |
| performance | load, prompt/decode time and tps, TTFT, p50/p95/p99 ILT, request latency, draft generated/accepted, repetitions/raw samples |
| resources | RSS/PSS/peak, shared/GPU memory with source, faults/swap, temperature/clock/power series, wall Wh, throttle flags |
| quality | deterministic comparison, perplexity/KLD or approved task score, errors/fallbacks, gate version |

**[RECOMMENDATION]** Compute ILT percentiles over raw content-token intervals after separating the first token. Report count and method; never pool machines, prompts, power modes, contexts, or cache states.

## Status vocabulary

- NOT_RUN: eligible planned cell without evidence.
- NOT_APPLICABLE: structurally unavailable, with reason/source.
- UNSUPPORTED_OBSERVED: exact command failed and logs are preserved.
- INVALID: control, correctness, sensor, or steady-state gate failed.
- MEASURED_CANDIDATE: raw evidence complete but not reviewed under Section 73.
- ACCEPTED: reviewed/promoted under approved methodology.

At publication every cell is NOT_RUN.

[S74-01]: sources.md#s74-01
[S74-02]: sources.md#s74-02
[S74-03]: sources.md#s74-03
[S74-04]: sources.md#s74-04
[S74-06]: sources.md#s74-06
[S74-07]: sources.md#s74-07
