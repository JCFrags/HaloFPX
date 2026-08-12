---
section_id: "78"
title: "Correctness, Regression, Determinism, and Model Quality Evaluation"
status: needs-machine-validation
last_verified: 2026-07-17
applies_to:
  repositories:
    - "ggml-org/llama.cpp@788e07dc91d266ad3162a1ce9037665656269689"
    - "charlie12345/ROCmFPX@a5605a72768c6562241b248e268e33dc92787394"
  software_versions: ["Reproducible Builds definition accessed 2026-07-17"]
  hardware_revisions: ["dual-Strix-Halo target; exact revisions open"]
related_sections: ["31", "54", "73", "74", "76", "77", "80", "81"]
---

# Correctness, Regression, Determinism, and Model Quality Evaluation

**[RECOMMENDATION]** This section defines the evidence required to call a HaloFPX change correct. It does not report a HaloFPX pass: no commands in this section were run on the target machines during this research pass.

**[RECOMMENDATION]** Correctness has layers. Byte-exact metadata, token IDs, protocol schemas, cache checksums, and state fingerprints must match exactly. Floating-point kernels use the comparator already attached to the operation and datatype in the pinned source. End-to-end model comparisons use logits, divergence, perplexity, and task behavior; generated text alone is too lossy to diagnose a regression.

**[INFERENCE]** Fixed seed is necessary but insufficient for a determinism claim because backend order, scheduling, state, and build identity can change execution. A replay identity also includes model and tokenizer hashes, chat template, runtime commit and build options, backend and topology, cache/state identity, sampler chain and order, request ordering, batching, and slot scheduling.

## Required evidence ladder

1. Unit and parser golden vectors.
2. Backend-operation comparisons against an approved reference.
3. Model-load, prompt-evaluation, decode, state-save/restore, and recurrent rollback tests.
4. Cross-backend and cross-rank logit comparisons on identical fixtures.
5. Quantization quality evaluation against the same unquantized baseline and tokenizer.
6. Protocol, JSON, tool-call, grammar, code, long-context retrieval, MTP acceptance, and recurrent-continuation suites.
7. Failure tests proving mismatches are rejected rather than silently accepted.

See [procedures_and_checks.md](procedures_and_checks.md) for the proposed execution matrix and [open_questions.md](open_questions.md) for the thresholds that still require calibration.
