---
section_id: "61"
title: "Continuation state facts and constraints"
status: "needs-machine-validation"
last_verified: "2026-07-16"
applies_to:
  repositories: ["ggml-org/llama.cpp@788e07dc91d266ad3162a1ce9037665656269689", "fewtarius/CachyLLama@6be745998f568e379ea197fcf827baec73ff9940"]
  software_versions: []
  hardware_revisions: []
related_sections: ["33", "35", "36"]
---

# Facts and constraints

| State family | Required contents | Current evidence |
|---|---|---|
| dense/GQA attention | K/V tensors, cells/pages, positions, sequence ownership, cache types | llama.cpp memory state APIs [S61-01] |
| MLA | compressed latent/rope-bearing cache representation and metadata | architecture-specific memory implementation; inventory required [S61-01] |
| sliding-window attention | retained cells plus absolute/logical positions and window policy | state must bind runtime configuration [S61-01] |
| recurrent/SSM/hybrid | recurrent tensors/cells plus any attention component | hybrid source writes both components [S61-01] |
| draft/MTP | draft context memory and position | Cachy `dft_data` [S61-02] |
| speculative controller | implementation type/version and deferred boundary/history state | Cachy `spec_data`; only some implementations expose state [S61-03] |
| sampler/processors | ordered chain, parameters, token history, adaptive accumulators | sampler chains are stateful and clone/reset/accept tokens [S61-04] |
| grammar | grammar implementation, grammar text/hash, parser stacks/accepted prefix | grammar participates in token acceptance [S61-04] |
| RNG | algorithm, complete engine state/counter, seed provenance | seed accessor exists; no general persistence contract identified [S61-01][S61-04] |
| server sequence | slot/session generation, tokens, positions, prompt checkpoints, request controls | server owns metadata outside model memory [S61-02] |

**[INFERENCE]** Reconstructing a sampler by seed and replay may work only if algorithm, chain order, token history, and RNG consumption are identical. It is not a version-independent serialization contract.

**[VERIFIED]** CachyLLama speculative state API has a TODO for more than one stateful speculative implementation [S61-03]. A single opaque `spec_data` stream therefore cannot be assumed to cover arbitrary compositions.

