---
section_id: "03"
title: "Glossary and Naming Open Questions"
status: "needs-machine-validation"
last_verified: "2026-07-16"
applies_to:
  repositories: ["Custom_Inference_Project"]
  software_versions: []
  hardware_revisions: []
related_sections: ["43", "49", "57"]
---

# Open questions

| ID | Question | Needed evidence | Impact |
|---|---|---|---|
| OQ-03-001 | Is HaloFPX the final public/project name? | User decision and collision/trademark check | Prefix stability |
| OQ-03-002 | What are the two nodes' canonical inventory IDs and aliases? | Machine inventory | Logs and experiment joins |
| OQ-03-003 | Which distributed execution enums will be implemented first? | ADR after benchmarks | API/schema stability |
| OQ-03-004 | What exact fields form cache compatibility identity? | Cache corruption/compatibility experiments | Correct reuse |
| OQ-03-005 | Is llama.cpp experimental `tensor` split retained by the selected upstream pin? | Exact commit/source inspection | Vocabulary and mode mapping |
| OQ-03-006 | Which tokenizer and chat-template artifacts are independently hashed? | Model loading design | Reproducibility |
| OQ-03-007 | Which metric definitions align with existing llama-bench/server output? | Tool output inspection | Benchmark comparability |

**[OPEN]** The glossary records working definitions, not proof that any proposed execution mode is correct or supported on the dual-node system.

## Follow-up research

- Extract native enum and log names from selected ROCmFPX/llama.cpp commits.
- Compare project terms with transport, cache, and operations section drafts for collisions.
- Add a machine-readable glossary only after consumers and validation needs are known.
