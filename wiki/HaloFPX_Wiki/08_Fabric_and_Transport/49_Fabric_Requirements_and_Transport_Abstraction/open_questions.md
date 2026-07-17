---
section_id: "49"
title: "Fabric Requirements - Open Questions"
status: "needs-machine-validation"
last_verified: "2026-07-16"
applies_to:
  repositories: ["ROCmFPX", "llama.cpp"]
  software_versions: []
  hardware_revisions: ["target machines unresolved"]
related_sections: ["09", "20", "38", "50", "52", "54", "55"]
---

# Open questions

| ID | Question | Resolution evidence |
|---|---|---|
| FT-49-Q1 | Are the two USB4 ports backed by independent controllers/links under simultaneous load? | FT-49-E1 plus Section 20 |
| FT-49-Q2 | What payload-size crossover separates latency and bulk policies? | FT-49-E2/Section 55 |
| FT-49-Q3 | Which distributed modes impose the tightest step deadline and largest activation? | Section 38 mode cost model |
| FT-49-Q4 | Which host/GPU buffer kinds can each carrier register safely? | Section 54 experiments |
| FT-49-Q5 | Is async progress a dedicated thread, cooperative polling, or hybrid? | CPU/latency measurements |
| FT-49-Q6 | What authentication and confidentiality are required on a physically local link? | threat model in Section 53 |
| FT-49-Q7 | What operation is authoritative after ambiguous reconnect: coordinator log, rank journal, or reconciliation query? | protocol decision/ADR |
| FT-49-Q8 | What p99/p99.9 SLOs are accepted for each traffic class? | Section 09 plus measured baseline |

**[OPEN]** Every question above remains unresolved; none is silently promoted to a requirement.

