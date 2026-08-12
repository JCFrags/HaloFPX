---
section_id: "41"
title: "Remote Speculation Open Questions"
status: "needs-machine-validation"
last_verified: "2026-07-16"
applies_to:
  repositories: ["HaloFPX design candidate"]
  software_versions: []
  hardware_revisions: []
related_sections: ["31", "38", "48", "52"]
---

# Open questions

| ID | **[OPEN]** question | Needed evidence |
|---|---|---|
| DR41-O1 | Which target/draft pairs are tokenizer and distribution compatible? | exact model inventory and corpus test |
| DR41-O2 | Is exact stochastic `q` metadata affordable, compressible losslessly, or better recomputed? | protocol prototype and p99/byte results |
| DR41-O3 | What draft depth maximizes accepted tokens per target time by workload? | `DR-41-E2` |
| DR41-O4 | Can draft batching reduce cost without p99 regression? | concurrency sweep |
| DR41-O5 | Which native MTP models/runtime paths exist in the selected fork? | section 31 source/model audit |
| DR41-O6 | Can the draft KV ingest the target-selected correction token cheaply? | runtime implementation test |
| DR41-O7 | What statistical equivalence/quality threshold is required? | section 48 decision |
| DR41-O8 | Is hidden-state traffic required for any desired EAGLE/MTP variant? | architecture-specific source audit |
