---
section_id: "41"
title: "Remote Draft-Node Speculation"
status: "needs-machine-validation"
last_verified: "2026-07-16"
applies_to:
  repositories: ["ggml-org/llama.cpp@788e07dc91d266ad3162a1ce9037665656269689", "vllm-project/vllm@9354f222042986addf20709e5274fc26e0d09745"]
  software_versions: []
  hardware_revisions: ["dual Strix Halo premise"]
related_sections: ["31", "38", "39", "46", "48", "52", "55"]
---

# 41 - Remote Draft-Node Speculation

**[RECOMMENDATION]** Keep the target model, authoritative sampler, committed tokens, and target KV on the coordinator/target node; run a compatible smaller drafter with its own KV on the second node. Exchange token IDs and only the probability metadata required by the declared sampling protocol.

**[OPEN]** No evidence yet shows remote drafting beats ordinary decode on this fabric/model set. Treat it as an experiment with immediate fallback to target-only decoding.

Pages: [facts](facts_and_constraints.md), [design](design_implications.md), [checks](procedures_and_checks.md), [questions](open_questions.md), [sources](sources.md).

## Critical correctness distinction

Greedy verification can communicate token proposals alone. Exact stochastic speculative sampling generally needs the draft distribution at a rejection position to sample from the correction distribution; sending only each proposed token's scalar probability is insufficient. Any compact approximation must be labeled as a changed sampling algorithm and quality-tested, not called exact.
