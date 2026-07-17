---
section_id: "60"
title: "System-Prompt Sharing, Deduplication, Copy-on-Write, and Continuations"
status: "needs-machine-validation"
last_verified: "2026-07-16"
applies_to:
  repositories: ["HaloKV design", "fewtarius/CachyLLama", "ggml-org/llama.cpp"]
  software_versions: ["CachyLLama 6be745998f568e379ea197fcf827baec73ff9940", "HaloKV proposal v0"]
  hardware_revisions: ["dual gfx1151 Strix Halo; exact hosts pending"]
related_sections: ["56", "57", "58", "59", "61", "63", "64"]
---

# System-Prompt Sharing, Deduplication, Copy-on-Write, and Continuations

## Decision summary

**[VERIFIED]** Pinned CachyLLama provides a global system-prefix pool keyed by 64-bit FNV-1a, stores only a bounded token prefix for collision checking, derives a boundary heuristically, and separates explicit-user caches from anonymous continuation scanning. [S60-01, S60-02, S60-03]

**[RECOMMENDATION]** HaloKV shares only immutable, fully token-verified prefix pages under an explicit sharing policy and complete section 57 fingerprint. Caller-supplied rendered token ranges replace role-marker heuristics. Conversation branches are copy-on-write DAG children. Continuations require authenticated tenant/session authorization; content similarity never grants access.

**[OPEN]** Safe global-prefix policy, exact tool/template boundaries, timing leakage, expiry/deletion semantics, recurrent-state sharing and performance have not been validated.

## Authoritative pages

- [Sharing facts and safety constraints](facts_and_constraints.md)
- [Prefix, branch and continuation design](design_implications.md)
- [Security/correctness procedures](procedures_and_checks.md)
- [Open questions](open_questions.md)
- [Sources](sources.md)

## Research split

Pinned-source inspection establishes predecessor behavior. The HaloKV policy is a recommendation awaiting threat review, exact model/template tests, two-host restoration experiments and sections 61/64 decisions.
