---
section_id: "78"
title: "Correctness, Regression, Determinism, and Model Quality Facts and Constraints"
status: "needs-machine-validation"
last_verified: "2026-07-17"
applies_to:
  repositories: ["ggml-org/llama.cpp@788e07dc91d266ad3162a1ce9037665656269689", "charlie12345/ROCmFPX@a5605a72768c6562241b248e268e33dc92787394"]
  software_versions: ["Reproducible Builds definition accessed 2026-07-17"]
  hardware_revisions: ["dual-Strix-Halo target; exact revisions open"]
related_sections: ["31", "54", "73", "74", "76", "77", "80", "81"]
---

# Facts and constraints

- **[VERIFIED]** Pinned `llama.cpp` includes unit tests for backend operations, grammars, JSON-schema conversion, chat templates, tokenization, save/load state, fragmented state restore, and recurrent-state rollback [S78-01].
- **[VERIFIED]** `test-backend-ops.cpp` uses operation-, datatype-, and backend-sensitive comparisons. Some comparisons use normalized mean squared error or mean absolute error, and Flash Attention tolerance varies with KV length [S78-02]. A single project-wide floating-point epsilon would discard this information.
- **[VERIFIED]** `llama-perplexity` supports perplexity comparison and KL-divergence analysis from saved logits. Its documentation warns that perplexity is not comparable across different models or tokenizers and that logit files can require tens of GiB [S78-03].
- **[VERIFIED]** Pinned `llama.cpp` exposes a seed option and resets sampler state after warm-up, but source support for a seed does not prove deterministic execution across different backends or distributed reduction orders [S78-01].
- **[VERIFIED]** Pinned ROCmFPX contains scripts for reference/backend operations, perplexity, agent JSON/grammar, tool use, long-context smoke, MTP/EAGLE/speculative decoding, and runtime regressions, plus a fragmented state-restore test [S78-05].
- **[VERIFIED]** ROCmFPX's aggregate script can report success while optional checks are omitted or individual model-dependent checks report skips [S78-05].
- **[INFERENCE]** A source test's existence is not evidence that either HaloFPX node passes it; target execution and retained output are required.
- **[RECOMMENDATION]** Quality comparisons must pin dataset revision, preprocessing, tokenizer, chat template, prompt bytes, context policy, model hash, runtime, backend, and topology.
- **[VERIFIED]** The pinned perplexity documentation states that results from different model/tokenizer identities are not comparable [S78-03].
- **[RECOMMENDATION]** Do not assume cross-backend floating-point equality. An allowed difference requires an approved reference distribution and must not mask token, schema, state, or protocol errors.
- **[RECOMMENDATION]** Any unexpected mismatch must fail closed, preserve the reproducer and evidence, and avoid promoting the candidate artifact.
- **[OPEN]** No HaloFPX correctness, determinism, or model-quality result was measured in this research pass.
