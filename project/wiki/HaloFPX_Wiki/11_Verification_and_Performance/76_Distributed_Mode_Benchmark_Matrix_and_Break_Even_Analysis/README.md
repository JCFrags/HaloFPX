---
section_id: "76"
title: "Distributed Mode Benchmark Matrix and Break-Even Analysis"
status: "needs-machine-validation"
last_verified: "2026-07-16"
applies_to:
  repositories:
    - "ggml-org/llama.cpp@788e07dc91d266ad3162a1ce9037665656269689"
    - "charlie12345/ROCmFPX@a5605a72768c6562241b248e268e33dc92787394"
  software_versions: []
  hardware_revisions: ["planned matched dual Strix Halo; exact revisions open"]
related_sections: ["29", "34", "36", "38", "40", "41", "42", "43", "44", "45", "46", "47", "48", "73", "74", "75", "78", "80"]
---

# Distributed Mode Benchmark Matrix and Break-Even Analysis

## Decision-useful summary

**[VERIFIED]** The pinned llama.cpp exposes RPC devices, tensor splitting, conventional draft-model speculation, and MTP draft modes. Its RPC documentation calls the backend proof-of-concept, fragile, and insecure. Availability is not evidence of correctness or benefit on HaloFPX. [S76-001][S76-002]

**[RECOMMENDATION]** Compare every distributed candidate against matched single-node and two-node replication controls. Freeze model bytes, tokenizer/template, prompt set, sampling, context, cache state, concurrency, power mode, and output-quality checks.

**[RECOMMENDATION]** Select a mode only within a declared workload region. Break-even is a model-, quant-, context-, batch-, concurrency-, cache-, and fabric-specific boundary with uncertainty, not one global tokens-per-second number.

**[OPEN]** No distributed HaloFPX mode is implemented and measured in this section. All equations are analysis templates; all thresholds require raw two-machine evidence.

## Retrieval map

- [Facts and constraints](facts_and_constraints.md) defines mode-specific costs.
- [Design implications](design_implications.md) defines the matrix and break-even rules.
- [Procedures and checks](procedures_and_checks.md) defines reproducible execution.
- [Open questions](open_questions.md) records blockers.
- [Sources](sources.md) pins primary evidence.

## Research split

1. **Completed source research:** pinned llama.cpp facilities and foundational speculative/tensor/pipeline/MoE methods.
2. **Required machine work:** single-node baselines, fabric characterization, mode implementations, matched workload runs, failure/degraded tests, and raw statistical analysis.
3. **Contingent decisions:** mode availability, plan selection, routing thresholds, model placement, microbatching, and fallback policy.
