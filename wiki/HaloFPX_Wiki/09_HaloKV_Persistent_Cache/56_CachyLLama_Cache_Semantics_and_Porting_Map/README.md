---
section_id: "56"
title: "CachyLLama Cache Semantics and Porting Map"
status: "needs-machine-validation"
last_verified: "2026-07-16"
applies_to:
  repositories: ["fewtarius/CachyLLama", "ggml-org/llama.cpp", "charlie12345/ROCmFPX"]
  software_versions: ["CachyLLama commit 6be745998f568e379ea197fcf827baec73ff9940", "llama.cpp commit 788e07dc91d266ad3162a1ce9037665656269689", "ROCmFPX commit a5605a72768c6562241b248e268e33dc92787394"]
  hardware_revisions: ["dual gfx1151 Strix Halo; exact hosts pending inventory"]
related_sections: ["57", "58", "59", "60", "61", "62", "63", "64", "65"]
---

# CachyLLama Cache Semantics and Porting Map

## Conclusion

**[VERIFIED]** CachyLLama implements persistent per-checkpoint files, hot/warm/cold memory tiers, token-prefix lookup, system-prefix files, kernel readahead hints, user namespaces, continuation discovery, compatibility hashes, and optional draft/speculative blobs at the pinned commit. [S56-01, S56-02, S56-03, S56-04, S56-05, S56-06]

**[RECOMMENDATION]** Port the semantic goals and tested llama-state integration points, not the storage format. HaloKV needs stronger content addressing, complete compatibility fingerprints, rank-local ownership, atomic publication, checksums, collision-resistant identity, explicit privacy policy, and corruption-as-miss behavior before any CachyLLama artifact is accepted.

**[OPEN]** No pinned CachyLLama checkpoint has been produced, fault-injected, restored on gfx1151, or compared against recomputation in this project.

## Authoritative pages

- [Facts and behavior map](facts_and_constraints.md)
- [Porting decisions](design_implications.md)
- [Source-audit and validation procedure](procedures_and_checks.md)
- [Open questions](open_questions.md)
- [Primary source records](sources.md)

## Research split

Source inspection is complete for the pinned heads. Runtime correctness, performance, recovery and two-rank behavior require the actual hosts. Storage-format and promotion decisions remain contingent on sections 57–65.
