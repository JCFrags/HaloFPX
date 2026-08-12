---
section_id: "74"
title: "Baseline matrix design implications"
status: "needs-machine-validation"
last_verified: "2026-07-16"
applies_to:
  repositories:
    - "charlie12345/ROCmFPX@a5605a72768c6562241b248e268e33dc92787394"
    - "ggml-org/llama.cpp@788e07dc91d266ad3162a1ce9037665656269689"
  software_versions: []
  hardware_revisions: ["two matched Strix Halo nodes; current equality unverified"]
related_sections: ["22", "29", "30", "31", "33", "36", "73", "76", "78", "79", "81"]
---

# Design implications

## Staged matrix

The dimensions form an impractically large full cross-product. **[RECOMMENDATION]** Preserve coverage and causal interpretation with staged advancement.

### Stage A - node/backend anchors

Run each row on nimo-1 and nimo-2, HIP and Vulkan, under the same named power mode.

| Axis | Required anchor levels |
|---|---|
| family | dense GQA (Qwen2.5-Coder-32B); MoE GQA (Qwen3-30B-A3B); hybrid SSM/MoE (Nemotron-3-Nano-30B-A3B); long-context dense (Mistral-Small-3.1-24B) [S74-05] |
| weight quant | one fitting quality reference; Q8_0; Q4_K_M; one architecture-approved ROCmFPX preset |
| depth | 0, 4096, 32768; add 65536/native-long only after allocation gate |
| batch/ubatch | 512/128, 2048/512 |
| K/V | f16/f16 |
| FA | off, on; record dispatch/failure; auto is a separate policy row |
| MTP | none |
| outputs | engine pp/tg plus end-to-end server |

**[OPEN]** Exact model hashes, fitting reference precision, and architecture-approved ROCmFPX presets are unresolved; Section 29 candidates are not validated.

### Stage B - isolated factors

Change one factor from a valid anchor:

- depths 0, 4K, 16K, 32K, then 64K, 128K, and native maximum subject to gates;
- batch/ubatch 128/64, 512/128, 1024/256, 2048/512;
- K/V f16/f16, q8_0/q8_0, q4_0/q4_0, plus ROCmFPX asymmetric/Turbo tuples only after Section 33 approval;
- FA off, on, auto, proving actual dispatch;
- every stable named power mode/limit exposed identically on both nodes;
- reference, Q8_0, Q4_K_M, and each architecture-qualified ROCmFPX weight candidate.

### Stage C - interactions and MTP

Test only hypothesized interactions: long-context x KV x FA; MoE x batch; quant x backend; power x sustained decode; MTP x depth x batch. For a compatible MTP artifact, compare none to draft-mtp with maximum draft counts 1, 2, 4, 8, recording generated/accepted drafts and quality. External draft is separate, not an MTP proxy [S74-07].

### Stage D - confirmation

Randomize valid control/candidate order within thermal blocks, use independent process starts, enough requests/tokens for tails, and repeat finalists on the other node. Final repetition/confidence rules depend on Section 73.

## Required comparisons

| Comparison | Decision | Invalid shortcut |
|---|---|---|
| node A vs B, identical tuple | interchangeability | pool first |
| HIP vs Vulkan | default/fallback | different builds or FA |
| prompt vs decode | workload mode | one combined tps |
| engine vs client | overhead/SLO | bench as TTFT |
| quant/KV vs reference | compression | speed without quality |
| cold vs warm | startup/steady service | undisclosed cache |
| shallow vs deep | context curve | large configured context with empty KV |
| burst vs sustained | thermal stability | best first minute |
| MTP none vs on | speculation value | acceptance rate alone |

## Promotion gates

**[RECOMMENDATION]** A tuple cannot become default unless both nodes execute without unexplained fallback; quality passes; raw samples/hashes/environment/sensors are complete; tail/sustained thresholds pass; memory headroom covers context/concurrency; steady thermals/clocks are demonstrated; and a fallback backend is qualified or its absence accepted as risk.

**[INFERENCE]** A backend that wins short engine decode can lose end-to-end or sustained service because sampling, scheduling, shared APU power, memory pressure, and thermals are outside or only partly inside llama-bench.

[S74-07]: sources.md#s74-07
[S74-05]: sources.md#s74-05
