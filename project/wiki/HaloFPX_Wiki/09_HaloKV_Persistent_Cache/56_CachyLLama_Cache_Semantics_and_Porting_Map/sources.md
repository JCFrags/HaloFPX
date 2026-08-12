---
section_id: "56"
title: "CachyLLama cache primary sources"
status: "draft"
last_verified: "2026-07-16"
applies_to:
  repositories: ["fewtarius/CachyLLama", "ggml-org/llama.cpp", "charlie12345/ROCmFPX"]
  software_versions: ["CachyLLama 6be745998f568e379ea197fcf827baec73ff9940"]
  hardware_revisions: ["dual gfx1151 Strix Halo; exact hosts pending inventory"]
related_sections: ["57", "58", "59", "60", "61", "62", "63", "64", "65"]
---

# CachyLLama cache primary sources

Access date: 2026-07-16. Repository paths are pinned to full commits.

| ID | Source | Claims supported | Limitations |
|---|---|---|---|
| S56-01 | fewtarius/CachyLLama [`README.md`](https://github.com/fewtarius/CachyLLama/blob/6be745998f568e379ea197fcf827baec73ff9940/README.md), commit `6be745998f568e379ea197fcf827baec73ff9940` | Intended persistence, tier, system-cache, recurrent and user behavior; CLI defaults | Project claims and illustrative performance, not independent measurement. |
| S56-02 | [`common/kv-ssd-cache.h`](https://github.com/fewtarius/CachyLLama/blob/6be745998f568e379ea197fcf827baec73ff9940/common/kv-ssd-cache.h) and [`.cpp`](https://github.com/fewtarius/CachyLLama/blob/6be745998f568e379ea197fcf827baec73ff9940/common/kv-ssd-cache.cpp), same commit | v3 records, tiers, lookup, continuation, hashes, prefetch, retention and files | Source inspection does not prove runtime correctness/durability. |
| S56-03 | [`common/kv-ssd-system-cache.h`](https://github.com/fewtarius/CachyLLama/blob/6be745998f568e379ea197fcf827baec73ff9940/common/kv-ssd-system-cache.h) and [`.cpp`](https://github.com/fewtarius/CachyLLama/blob/6be745998f568e379ea197fcf827baec73ff9940/common/kv-ssd-system-cache.cpp), same commit | System-prefix format, hash verification boundary, retention, sync and boundary heuristic | No universal template correctness or atomic-publication proof. |
| S56-04 | [`docs/development/user-isolation-design.md`](https://github.com/fewtarius/CachyLLama/blob/6be745998f568e379ea197fcf827baec73ff9940/docs/development/user-isolation-design.md), same commit | Explicit/anonymous namespaces, concurrency, privacy intent and integration files | Design document can drift from code; authentication remains external. |
| S56-05 | [`tools/server/server-context-page-manager.cpp`](https://github.com/fewtarius/CachyLLama/blob/6be745998f568e379ea197fcf827baec73ff9940/tools/server/server-context-page-manager.cpp), same commit | Server lookup, user routing and cache orchestration | Coupled to current server/slot model. |
| S56-06 | [`tools/server/server-context-ssd-cache.cpp`](https://github.com/fewtarius/CachyLLama/blob/6be745998f568e379ea197fcf827baec73ff9940/tools/server/server-context-ssd-cache.cpp) and [`server-context.cpp`](https://github.com/fewtarius/CachyLLama/blob/6be745998f568e379ea197fcf827baec73ff9940/tools/server/server-context.cpp), same commit | Save/restore integration, target/draft/spec state flow | Needs dynamic path proof. |
| S56-07 | [`include/llama.h`](https://github.com/fewtarius/CachyLLama/blob/6be745998f568e379ea197fcf827baec73ff9940/include/llama.h), [`src/llama-context.cpp`](https://github.com/fewtarius/CachyLLama/blob/6be745998f568e379ea197fcf827baec73ff9940/src/llama-context.cpp), same commit | Sequence-state API, recurrent snapshots, partial/on-device flags | Fork API; not assumed ABI-compatible with other pins. |
| S56-08 | ggml-org/llama.cpp [commit `788e07dc`](https://github.com/ggml-org/llama.cpp/commit/788e07dc91d266ad3162a1ce9037665656269689) and charlie12345/ROCmFPX [commit `a5605a72`](https://github.com/charlie12345/ROCmFPX/commit/a5605a72768c6562241b248e268e33dc92787394) | Exact comparison/integration heads observed | Heads are not an approved merge base; full ancestry review is open. |

## Source conflicts

**[OPEN]** README wording can imply fully durable, broadly compatible restore, while the inspected identity, verification and publication mechanisms leave collision, partial-token verification, template-boundary and crash-safety questions. This section preserves that conflict rather than promoting the README claim.

