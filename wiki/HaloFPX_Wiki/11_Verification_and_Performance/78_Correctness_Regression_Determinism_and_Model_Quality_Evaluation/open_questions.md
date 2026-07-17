---
section_id: "78"
title: "Correctness, Regression, Determinism, and Model Quality Open Questions"
status: "needs-machine-validation"
last_verified: "2026-07-17"
applies_to:
  repositories: ["ggml-org/llama.cpp@788e07dc91d266ad3162a1ce9037665656269689", "charlie12345/ROCmFPX@a5605a72768c6562241b248e268e33dc92787394"]
  software_versions: ["Reproducible Builds definition accessed 2026-07-17"]
  hardware_revisions: ["dual-Strix-Halo target; exact revisions open"]
related_sections: ["31", "54", "73", "74", "76", "77", "80", "81"]
---

# Open questions

1. [OPEN] Which model families, sizes, architectures, and exact hashes form the release qualification set?
2. [OPEN] Which licensed datasets and revisions cover perplexity, code, JSON, tools, grammar, retrieval, and recurrent behavior?
3. [OPEN] What repeated-run distribution supports cross-backend and cross-rank logit thresholds for each datatype and operation family?
4. [OPEN] Are Section 31's proposed relative-perplexity tiers acceptable for each release class, or must they be model-specific?
5. [OPEN] Which tests are mandatory on every change, nightly hardware, and release candidate runs?
6. [OPEN] Can any supported same-configuration path guarantee byte-identical logits, or only exact token continuation?
7. [OPEN] What scheduling controls are necessary to make multi-session seeded replay meaningful?
8. [OPEN] What state/cache identity fields are mandatory for compatibility and corruption rejection?
9. [OPEN] What acceptance-rate and speed evidence is required before enabling MTP/EAGLE by default without trading quality?
10. [OPEN] Which recurrent architectures and fragmentation patterns must be represented?
