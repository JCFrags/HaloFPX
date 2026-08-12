---
section_id: "49"
title: "Fabric Requirements and Transport Abstraction"
status: "needs-machine-validation"
last_verified: "2026-07-16"
applies_to:
  repositories: ["charlie12345/ROCmFPX@a5605a72768c6562241b248e268e33dc92787394", "ggml-org/llama.cpp@788e07dc91d266ad3162a1ce9037665656269689"]
  software_versions: ["Linux 7.2-rc3 source snapshot fce2dfa773ced15f27dd27cd0b482a7473cdcf2a"]
  hardware_revisions: ["dual Strix Halo premise; exact BOM unresolved"]
related_sections: ["09", "20", "38", "39", "50", "51", "52", "53", "54", "55"]
---

# 49 - Fabric Requirements and Transport Abstraction

**[RECOMMENDATION]** Put a versioned, message-oriented HaloFPX fabric API above every concrete carrier. The API should expose lanes, deadlines, cancellation, registered-buffer handles, asynchronous completions, path health, and single-link fallback without promising capabilities the carrier cannot provide.

**[ASSUMPTION]** Two independently usable USB4 host-to-host paths exist. Section 20 and on-machine experiment `FT-49-E1` must prove this before dual-link requirements become acceptance criteria.

## Authoritative pages

- [Facts and constraints](facts_and_constraints.md)
- [Design implications](design_implications.md)
- [Procedures and checks](procedures_and_checks.md)
- [Open questions](open_questions.md)
- [Sources](sources.md)

No performance result is claimed here. Numeric SLOs remain provisional until Section 55 measurements.

