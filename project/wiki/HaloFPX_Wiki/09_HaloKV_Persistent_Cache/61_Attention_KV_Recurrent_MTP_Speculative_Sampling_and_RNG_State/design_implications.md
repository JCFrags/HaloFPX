---
section_id: "61"
title: "Continuation stream design"
status: "needs-machine-validation"
last_verified: "2026-07-16"
applies_to:
  repositories: []
  software_versions: []
  hardware_revisions: ["dual Strix Halo"]
related_sections: ["57", "58", "59", "63"]
---

# Design implications

**[RECOMMENDATION]** One generation manifest should reference independently checksummed streams:

`tokens`; `sequence-meta`; `target-memory`; `draft-memory`; `spec-controller/<type>`; `sampler-chain`; `grammar`; `rng`; `logits-processors`; `server-request-state`.

Each descriptor needs stream type, schema version, producer commit, required/optional flag, length, digest, logical position, rank/layer ownership, and compatibility fingerprint.

**[RECOMMENDATION]** Restore order is compatibility -> tokens/sequence -> target memory -> draft memory -> speculative controllers -> sampler/grammar/processors/RNG -> server metadata. Do not expose the session until every required stream validates.

**[RECOMMENDATION]** Permit partial reuse only through explicit policy: for example, valid target memory may be reused while speculative/draft state is discarded and rebuilt. Never claim exact replay if sampler/RNG/grammar state is missing.

**[INFERENCE]** Rank-local model memory can be sharded, but user-visible sampler and grammar state should normally have one coordinator owner to avoid divergent token acceptance.

