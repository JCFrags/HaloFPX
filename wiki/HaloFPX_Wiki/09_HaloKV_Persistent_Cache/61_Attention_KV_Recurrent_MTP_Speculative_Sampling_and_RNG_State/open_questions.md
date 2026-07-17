---
section_id: "61"
title: "Continuation state open questions"
status: "needs-machine-validation"
last_verified: "2026-07-16"
applies_to:
  repositories: []
  software_versions: []
  hardware_revisions: []
related_sections: ["35", "36", "57"]
---

# Open questions

| ID | Question | Evidence needed |
|---|---|---|
| O61-01 | Which memory types/flags are used by each target GGUF? | M61-01 |
| O61-02 | Does target-context state include any sampler/RNG state? | byte/schema and replay test |
| O61-03 | Can every enabled speculative implementation serialize state? | exact implementation matrix |
| O61-04 | Is seed-plus-token replay sufficient for each sampler chain? | M61-02 |
| O61-05 | How is grammar/parser state serialized across versions? | schema design and tests |
| O61-06 | Which streams may be discarded and rebuilt safely? | fault/rebuild equivalence |
| O61-07 | Who owns coordinator state during rank failover? | section 58 protocol |
| O61-08 | What equality/tolerance defines exact continuation per backend? | section 78 decision |

