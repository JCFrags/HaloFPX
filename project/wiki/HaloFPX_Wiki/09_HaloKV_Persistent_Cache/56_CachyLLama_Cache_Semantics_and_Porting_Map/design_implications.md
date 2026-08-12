---
section_id: "56"
title: "CachyLLama porting decisions and integration hooks"
status: "needs-machine-validation"
last_verified: "2026-07-16"
applies_to:
  repositories: ["fewtarius/CachyLLama", "ggml-org/llama.cpp", "charlie12345/ROCmFPX"]
  software_versions: ["CachyLLama 6be745998f568e379ea197fcf827baec73ff9940"]
  hardware_revisions: ["dual gfx1151 Strix Halo; exact hosts pending inventory"]
related_sections: ["57", "58", "59", "60", "61", "62", "63", "64", "65"]
---

# CachyLLama porting decisions and integration hooks

## Retain, redesign, reject

| Decision | Items |
|---|---|
| Retain | llama sequence-state save/load integration; explicit attention/recurrent/draft/spec component boundaries; reusable verified prefixes; tier and prefetch concepts; rank-local reads; user-scoped routing; incompatibility as miss. |
| Redesign | checkpoint IDs; full compatibility fingerprint; immutable page/segment format; index and prefix DAG; atomic commit/recovery; collision-resistant content IDs; quotas/GC; longest-prefix lookup; continuation authorization; async I/O; component schemas. |
| Reject | durable slot IDs; weak FNV identity as trust decision; fuzzy cross-directory restore; partial-prefix trust; heuristic role detection as cache boundary; unbounded cold retention; silent partial-state acceptance; cache transfer between ranks as normal restore. |

## Source integration map

| Pinned CachyLLama hook | HaloKV boundary |
|---|---|
| `common/kv-ssd-cache.{h,cpp}` | Reference behavior only; replace persistence/index/tiering implementation. |
| `common/kv-ssd-system-cache.{h,cpp}` | Extract explicit caller-supplied prefix boundaries; replace hashing/files/retention. |
| `tools/server/server-context-page-manager.{h,cpp}` | Keep request/slot orchestration seam; call HaloKV lookup/restore/commit APIs. |
| `tools/server/server-context-ssd-cache.{h,cpp}` | Migration adapter for CachyLLama artifacts, never trusted native storage. |
| `tools/server/server-context.cpp` | Carry tenant, request, rank and topology identities; surface diagnostic miss reasons. |
| `tools/server/server-task.{h,cpp}` / `server-chat.cpp` | Authenticate tenant identity outside prompt content; do not accept user-provided namespace without authorization. |
| `include/llama.h`, `src/llama-context.cpp`, recurrent tests | Versioned runtime adapter for state components and suffix replay. |

## Required HaloKV API semantics

```text
lookup(prefix_tokens, tenant, compatibility, topology, rank) -> verified candidate or miss reason
restore(candidate, expected_components) -> all-required-components valid or miss
commit(parent, token_range, components, provenance) -> immutable checkpoint ID
branch(parent, suffix) -> copy-on-write lineage
inspect/check/migrate/gc -> offline-safe administrative operations
```

**[RECOMMENDATION]** Reads must verify header/schema, digest, exact token range, compatibility, tenant authorization, rank ownership and required component set before publication to a live slot. Corruption or ambiguity is a miss/recompute, never best-effort acceptance.

**[RECOMMENDATION]** In two-rank operation each rank restores its own locally owned state from NVMe and exchanges only a bounded readiness manifest. On topology mismatch or rank failure, reject distributed restore and use the defined single-node/recompute path (section 58).

