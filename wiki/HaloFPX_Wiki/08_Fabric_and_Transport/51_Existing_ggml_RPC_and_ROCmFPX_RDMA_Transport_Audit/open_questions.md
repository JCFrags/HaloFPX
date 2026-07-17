---
section_id: "51"
title: "ggml RPC and RDMA Audit - Open Questions"
status: "needs-machine-validation"
last_verified: "2026-07-17"
applies_to:
  repositories: ["ROCmFPX@a5605a7", "llama.cpp@788e07d"]
  software_versions: ["libibverbs"]
  hardware_revisions: []
related_sections: ["11", "15", "49", "50", "54", "55"]
---

# Open questions

| ID | Question | Needed evidence |
|---|---|---|
| FT-51-Q1 | What upstream commit introduced the current verbs path, and what review/tests accompany it? | full history/PR review, not snapshot inference |
| FT-51-Q2 | Does the target USB4 networking stack expose any verbs/RDMA device? | `ibv_devices`, kernel/device inventory |
| FT-51-Q3 | What exact build/ABI tuple is wire-compatible? | mixed-version matrix and protocol specification |
| FT-51-Q4 | Can graph reuse support multiple sessions/graphs safely? | concurrent UID and reconnect tests |
| FT-51-Q5 | Is the unguarded RDMA completion length reachable from a peer/fault? | sanitizer and fault injection |
| FT-51-Q6 | Does hash cache include adequate namespace, length, integrity and atomicity? | cache code audit and corruption tests |
| FT-51-Q7 | What speedup, if any, survives host staging copies and one-at-a-time completions? | matched benchmark |
| FT-51-Q8 | Which components are upstream-bound versus HaloFPX-only? | patch-stack decision in Section 15 |
| FT-51-Q9 | Do both deployed peers prove fixed-source executable/library provenance and intended-address-only exposure? | FT-51-E0 retained artifact/network/privilege receipt |

**[OPEN]** The source audit supports a bring-up decision, not production transport acceptance.
