---
section_id: "58"
title: "Rank-Local Ownership and Distributed Restore Coordination"
status: "needs-machine-validation"
last_verified: "2026-07-16"
applies_to:
  repositories: ["charlie12345/ROCmFPX", "fewtarius/CachyLLama", "ggml-org/llama.cpp"]
  software_versions: ["ROCmFPX a5605a72768c6562241b248e268e33dc92787394", "CachyLLama 6be745998f568e379ea197fcf827baec73ff9940"]
  hardware_revisions: ["two gfx1151 Strix Halo hosts; exact topology pending"]
related_sections: ["49", "50", "52", "54", "56", "57", "59", "61", "63", "75"]
---

# Rank-Local Ownership and Distributed Restore Coordination

## Decision summary

**[VERIFIED]** Pinned ROCmFPX exposes layer, row and tensor split modes plus remote ggml RPC devices, while its llama sequence-state APIs serialize runtime-owned sequence state. Pinned CachyLLama persists one server context's target/draft/spec sequence blobs; it does not implement a two-rank HaloKV restore protocol. [S58-01, S58-02, S58-03, S58-04]

**[RECOMMENDATION]** A distributed checkpoint is a small committed manifest that names immutable rank-local component objects. Each rank reads only its own NVMe objects, verifies them, stages state, and participates in an all-ready generation gate. Missing, corrupt, mismatched or timed-out ranks cause a miss/recompute or documented single-node fallback—never partial publication.

**[OPEN]** No exact HaloFPX execution plan, rank-state partition, restore latency, failure timeout or safe suffix-replay boundary has been measured.

## Authoritative pages

- [Ownership facts and constraints](facts_and_constraints.md)
- [Restore protocol and plan semantics](design_implications.md)
- [Validation and fault-injection procedure](procedures_and_checks.md)
- [Open questions](open_questions.md)
- [Primary sources](sources.md)

## Research split

Pinned source inspection defines current APIs and missing protocol. On-machine work must prove ownership maps, local serialization, independent reads, readiness timing, suffix replay and failure behavior for each execution plan.
