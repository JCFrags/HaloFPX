---
section_id: "34"
title: "MoE open questions"
status: "needs-machine-validation"
last_verified: "2026-07-16"
applies_to:
  repositories: []
  software_versions: []
  hardware_revisions: ["dual Strix Halo"]
related_sections: ["29", "44", "47", "52"]
---

# Open questions

| ID | Question | Resolution evidence |
|---|---|---|
| O34-01 | Which target model revisions and quantizations are in scope? | section 29 model/hash ledger |
| O34-02 | Does a CachyLLama/llama-ai branch already export expert telemetry? | exact branch/commit and schema |
| O34-03 | How stable are hot experts across prompt/decode, domains, and batch sizes? | M34-01 repeated traces |
| O34-04 | What is per-layer expert working-set size and reuse distance? | trace plus tensor inventory |
| O34-05 | Is expert replication better than whole-layer placement over dual USB4? | M34-02 plus section 52 fabric measurements |
| O34-06 | What instrumentation overhead is acceptable? | declared SLO and disabled/enabled A/B |
| O34-07 | How are rank failure and owner-map changes handled? | section 44 protocol and single-node fallback test |

**[OPEN]** No item above is resolved by paper architecture alone.

