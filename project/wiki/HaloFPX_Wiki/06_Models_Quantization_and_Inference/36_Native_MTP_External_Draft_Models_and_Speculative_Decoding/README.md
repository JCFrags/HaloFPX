---
section_id: "36"
title: "Native MTP, External Draft Models, and Speculative Decoding"
status: "needs-machine-validation"
last_verified: "2026-07-16"
applies_to:
  repositories: ["ggml-org/llama.cpp@788e07dc91d266ad3162a1ce9037665656269689", "charlie12345/ROCmFPX@a5605a72768c6562241b248e268e33dc92787394"]
  software_versions: []
  hardware_revisions: ["dual Strix Halo"]
related_sections: ["29", "41", "45", "73", "76"]
---

# 36 - MTP and speculative decoding

Speculation is an optional scheduling optimization. The target model remains the authority for emitted tokens, and a claimed speedup is valid only when output distribution/quality, workload, and sampling policy are matched.

- **[VERIFIED]** Speculative decoding drafts several tokens and verifies them in a target-model batch; the original algorithm preserves the target distribution when acceptance/resampling rules are followed [S36-03].
- **[VERIFIED]** Current llama.cpp supports external draft models and native `draft-mtp`, plus acceptance counters and tunable draft depth/probability thresholds [S36-01].
- **[VERIFIED]** DeepSeek-V3 trains auxiliary MTP modules and publishes them as additional weights; this is model-native drafting, not a generic external model [S36-04][S36-05].
- **[INFERENCE]** A second Strix Halo helps only if draft generation plus link/control latency is hidden beneath target verification savings.
- **[RECOMMENDATION]** Keep no-speculation as a first-class fallback and select modes from a measured workload matrix.

