---
section_id: "62"
title: "Async I/O pipeline design"
status: "needs-machine-validation"
last_verified: "2026-07-16"
applies_to:
  repositories: []
  software_versions: []
  hardware_revisions: ["dual Strix Halo"]
related_sections: ["54", "59", "63"]
---

# Design implications

## Proposed pipeline

**[RECOMMENDATION]** `tokenize -> prefix candidates -> async metadata lookup -> bounded prefetch -> verify manifest/digests -> stage required streams -> backend restore -> publish hit`. Cancellation invalidates an operation generation; late completions may populate a safe shared read cache but never mutate a reused session.

## Tiers and budgets

| Tier | Contents | Policy |
|---|---|---|
| active | currently bound backend/session state | protected, hard budget |
| hot DRAM | verified decompressed/staging pages | request-aware LRU, pinned during restore |
| warm DRAM/page cache | likely prefixes and immutable bytes | reclaimable, no correctness dependency |
| SSD | immutable segments/manifests | durability and quota policy |

**[RECOMMENDATION]** Budget active + registered buffers + hot + warm + backend allocations against cgroup/system memory, with reserve and hysteresis. On pressure, cancel speculative prefetch before active work.

## I/O modes

**[RECOMMENDATION]** Use separate queue/credit classes for foreground reads, commit writes/fsync, prefetch, and GC. Bound bytes and operations, not only queue depth. Prefer buffered I/O initially; test `O_DIRECT` with queried alignment and registered aligned buffers. Keep fallback when io_uring operations/features are unavailable.

**[RECOMMENDATION]** Delay low-priority writeback until response streaming starts only in performance/turn-durable modes; strict mode must follow section 63's acknowledgement boundary.

