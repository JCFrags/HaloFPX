---
section_id: "31"
title: "Conversion, Imatrix, Calibration, and Quality Validation"
status: "needs-machine-validation"
last_verified: "2026-07-16"
applies_to:
  repositories: ["charlie12345/ROCmFPX", "ggml-org/llama.cpp"]
  software_versions: ["ROCmFPX a5605a7", "llama.cpp 788e07d"]
  hardware_revisions: []
related_sections: ["29", "30", "76", "78"]
---

# Conversion and quality gate

The authoritative flow is `immutable checkpoint -> BF16/F16 GGUF -> imatrix -> quant candidates -> matched quality and backend validation`. **[VERIFIED]** Current llama.cpp documents conversion before quantization and warns that requantization can severely reduce quality [S31-01].

**[RECOMMENDATION]** Never publish a quant because it loads. Publish only with source, tokenizer, converter, imatrix, recipe, output hashes, and matched acceptance evidence.

## Research split

- Completed now: pinned tool behavior and safe commands, GGUF imatrix/resume behavior, quantizer controls, provenance fields, proposed acceptance tiers.
- Machine work: disk/RAM sizing, actual conversion, source-vs-GGUF logits, calibration, perplexity/KLD/task suites, interruption recovery, both backends/nodes.
- Contingent: final thresholds, calibration mixture, protected tensors, and promoted recipes.

