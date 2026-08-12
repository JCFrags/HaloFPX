---
section_id: "36"
title: "Speculative decoding design implications"
status: "needs-machine-validation"
last_verified: "2026-07-16"
applies_to:
  repositories: ["ggml-org/llama.cpp@788e07dc91d266ad3162a1ce9037665656269689"]
  software_versions: []
  hardware_revisions: ["dual Strix Halo"]
related_sections: ["41", "45", "52", "76"]
---

# Design implications

## Controller invariants

**[RECOMMENDATION]** The coordinator must assign monotonically increasing proposal IDs containing session generation, target prefix length/hash, draft tokens, draft probabilities when needed, and configuration fingerprint. Verification responses bind the same ID and the accepted prefix length. Stale or duplicate responses are discarded.

**[RECOMMENDATION]** The target rank alone commits emitted tokens. The draft rank may run ahead only within bounded credits; after rejection it rolls back/rebuilds from the committed target prefix.

**[RECOMMENDATION]** On link loss, draft crash, state mismatch, or timeout, stop accepting proposals and continue target-only. This is the required single-node fallback.

## Mode selection

**[INFERENCE]** Native MTP shares model lineage and may avoid an independent draft model, but it consumes extra weights/context and can add kernels. External drafting can isolate onto node 2 but adds model memory and transport. Neither dominates without measurement.

**[RECOMMENDATION]** Key autotuner inputs: prompt/decode phase, context length, requested output length, batch/slot count, temperature/top-p/top-k, grammar/tool constraints, recent acceptance length distribution, target verification latency, draft latency, link RTT/bytes, and memory headroom.

**[ASSUMPTION]** Remote draft proposals can overlap target work. Section 41 must demonstrate this scheduling behavior; otherwise network drafting is serialized overhead.

## Quality contract

**[RECOMMENDATION]** For greedy decoding require token-for-token identity. For stochastic decoding compare paired distributional tests under an implementation documented as lossless; do not require identical random draws unless RNG consumption is intentionally synchronized. Grammar-constrained, tool-call, multimodal, and logprobs requests each need explicit support or target-only fallback.

