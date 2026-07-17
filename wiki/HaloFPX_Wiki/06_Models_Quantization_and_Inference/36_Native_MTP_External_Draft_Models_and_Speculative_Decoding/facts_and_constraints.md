---
section_id: "36"
title: "Speculative decoding facts and constraints"
status: "needs-machine-validation"
last_verified: "2026-07-16"
applies_to:
  repositories: ["ggml-org/llama.cpp@788e07dc91d266ad3162a1ce9037665656269689", "charlie12345/ROCmFPX@a5605a72768c6562241b248e268e33dc92787394"]
  software_versions: []
  hardware_revisions: []
related_sections: ["29", "41"]
---

# Facts and constraints

## Modes

| Mode | Draft state | Verification authority | Primary evidence |
|---|---|---|---|
| none | none | target autoregressive step | baseline |
| external draft | separate model context/cache, tokenizer compatibility required | target model | [S36-01][S36-03] |
| native MTP | target-family auxiliary NextN/MTP weights and context | target main head | [S36-01][S36-04][S36-05] |

**[VERIFIED]** llama.cpp's current documentation lists `draft-simple` and `draft-mtp`, draft model/device/offload controls, maximum/minimum draft tokens, and probability thresholds [S36-01]. The implementation records generated and accepted drafts and maintains target/draft contexts [S36-02].

**[VERIFIED]** DeepSeek-V3 uses a multi-token prediction objective; its published repository describes 671B main-model weights plus a 14B MTP module [S36-04][S36-05]. This does not imply every MTP-format model is compatible with llama.cpp.

## Sampling and quality

**[VERIFIED]** The lossless speculative algorithm accepts draft samples according to target/draft probabilities and samples from a corrected distribution on rejection [S36-03]. Greedy equality is a simpler special case. Implementation-specific shortcuts must be evaluated against their stated contract.

**[INFERENCE]** Acceptance rate alone is insufficient: throughput also depends on draft cost, verification batch efficiency, average accepted run length, rejection recovery, memory pressure, and host/link synchronization.

## State

**[RECOMMENDATION]** Serialize mode, model hashes, tokenizer/vocabulary identity, draft depth/thresholds, target and draft sequence positions, both cache/state fingerprints, sampler/RNG state when replay is claimed, and accepted/generated counters. Restore must either recover a consistent pair or disable speculation and rebuild draft state from the verified target prefix.

**[OPEN]** Current upstream state APIs do not by themselves prove a portable, atomic cross-process serialization of the whole speculative controller.

