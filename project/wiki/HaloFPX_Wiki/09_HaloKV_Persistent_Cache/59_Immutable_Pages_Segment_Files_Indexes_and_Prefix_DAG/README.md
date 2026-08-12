---
section_id: "59"
title: "Immutable Pages, Segment Files, Indexes, and Prefix DAG"
status: "needs-machine-validation"
last_verified: "2026-07-16"
applies_to:
  repositories: ["HaloKV design", "fewtarius/CachyLLama"]
  software_versions: ["CachyLLama 6be745998f568e379ea197fcf827baec73ff9940", "format proposal v0"]
  hardware_revisions: ["dual Strix Halo NVMe hosts; exact filesystems pending"]
related_sections: ["57", "58", "60", "61", "62", "63", "64", "65"]
---

# Immutable Pages, Segment Files, Indexes, and Prefix DAG

## Proposed format direction

**[RECOMMENDATION]** HaloKV should store immutable content-addressed pages inside append-only segment files; publish checkpoint/prefix DAG nodes through a crash-recoverable metadata transaction; and treat all indexes as rebuildable accelerators. Every page and manifest is length-delimited, versioned and cryptographically checksummed.

**[VERIFIED]** CachyLLama v3 instead uses mutable index metadata and one final-path file per checkpoint with a fixed record plus concatenated blobs. It is a source behavior reference, not a suitable HaloKV disk format. [S59-01]

**[OPEN]** Page size, alignment, segment target, metadata engine, buffered/direct/mmap read path and compaction policy have not been benchmarked on the project NVMe/filesystems.

## Authoritative pages

- [Format facts and constraints](facts_and_constraints.md)
- [Schemas and publication design](design_implications.md)
- [Prototype and fault checks](procedures_and_checks.md)
- [Open questions](open_questions.md)
- [Primary sources](sources.md)

## Research split

Primary sources establish filesystem/database primitives and pinned predecessor behavior. The proposed v0 schema is a recommendation. Machine experiments and section 63 crash testing must precede format approval.

