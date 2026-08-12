---
section_id: "14"
title: "Sources"
status: "needs-machine-validation"
last_verified: "2026-07-16"
applies_to:
  repositories:
    - "fewtarius/llama-ai@1017f3dfdce3ca2b06aa9007b23295db3bb35722"
    - "fewtarius/CachyLLama@6be745998f568e379ea197fcf827baec73ff9940"
    - "ggml-org/llama.cpp@92366df30d4eaa4b85139b5fd694360237731b19"
  software_versions: []
  hardware_revisions: []
related_sections: ["11", "13", "15", "16"]
---

# Sources

Access date for Internet sources: 2026-07-16. All repository links are pinned; branch pages are intentionally not used as evidence.

## S14-001 — llama-ai frozen wrapper

- **Title/publisher:** `fewtarius/llama-ai`, commit `1017f3dfdce3ca2b06aa9007b23295db3bb35722`
- **Revision/date:** commit authored 2026-07-08; no release tag found
- **URL:** <https://github.com/fewtarius/llama-ai/tree/1017f3dfdce3ca2b06aa9007b23295db3bb35722>
- **Key files:** [README](https://github.com/fewtarius/llama-ai/blob/1017f3dfdce3ca2b06aa9007b23295db3bb35722/README.md), [runner](https://github.com/fewtarius/llama-ai/blob/1017f3dfdce3ca2b06aa9007b23295db3bb35722/llama-run.sh), [benchmark harness](https://github.com/fewtarius/llama-ai/blob/1017f3dfdce3ca2b06aa9007b23295db3bb35722/scripts/benchmark.sh), [systemd example](https://github.com/fewtarius/llama-ai/blob/1017f3dfdce3ca2b06aa9007b23295db3bb35722/systemd/llama-server.service), [.gitmodules](https://github.com/fewtarius/llama-ai/blob/1017f3dfdce3ca2b06aa9007b23295db3bb35722/.gitmodules)
- **Supports:** gitlink, operational structure, profiles, flags, runner behavior, benchmark procedure, stated platform limitations, GPL license.
- **Limitations/conflicts:** README performance statements are project-authored; profiles are policy; service example uses a stale flag and fixed paths.

## S14-002 — CachyLLama frozen engine fork

- **Title/publisher:** `fewtarius/CachyLLama`, commit `6be745998f568e379ea197fcf827baec73ff9940`
- **Revision/date:** merge commit 2026-07-08; no release tag found
- **URL:** <https://github.com/fewtarius/CachyLLama/tree/6be745998f568e379ea197fcf827baec73ff9940>
- **Key files:** [SSD cache](https://github.com/fewtarius/CachyLLama/blob/6be745998f568e379ea197fcf827baec73ff9940/common/kv-ssd-cache.cpp), [format](https://github.com/fewtarius/CachyLLama/blob/6be745998f568e379ea197fcf827baec73ff9940/common/kv-ssd-cache.h), [system cache](https://github.com/fewtarius/CachyLLama/blob/6be745998f568e379ea197fcf827baec73ff9940/common/kv-ssd-system-cache.cpp), [server lifecycle](https://github.com/fewtarius/CachyLLama/blob/6be745998f568e379ea197fcf827baec73ff9940/tools/server/server-context.cpp), [page-manager test](https://github.com/fewtarius/CachyLLama/blob/6be745998f568e379ea197fcf827baec73ff9940/test_kv_page_manager.cpp), [hybrid memory](https://github.com/fewtarius/CachyLLama/blob/6be745998f568e379ea197fcf827baec73ff9940/src/llama-memory-hybrid.cpp), [C API](https://github.com/fewtarius/CachyLLama/blob/6be745998f568e379ea197fcf827baec73ff9940/include/llama.h)
- **Supports:** cache format/tiering/matching, durability behavior, recurrent/MTP handling, server wiring, user scheduling, telemetry, test inventory, MIT license.
- **Limitations/conflicts:** code inspection proves implementation presence, not correctness on HaloFPX; README defaults and claims sometimes differ from code.

## S14-003 — exact merged upstream baseline

- **Title/publisher:** `ggml-org/llama.cpp`, commit `92366df30d4eaa4b85139b5fd694360237731b19`
- **Revision/date:** 2026-07-08
- **URL:** <https://github.com/ggml-org/llama.cpp/commit/92366df30d4eaa4b85139b5fd694360237731b19>
- **Comparison:** <https://github.com/fewtarius/CachyLLama/compare/92366df30d4eaa4b85139b5fd694360237731b19...6be745998f568e379ea197fcf827baec73ff9940>
- **Supports:** correct upstream parent and fork-delta boundary.
- **Limitations:** a later upstream branch is not the frozen comparison base; the GitHub comparison includes merge topology and should be paired with local `git diff`.

## S14-004 — repository benchmark evidence

- **Title/publisher:** `fewtarius/llama-ai` checked-in benchmark corpus and harness
- **Revision/date:** as contained in `1017f3d`; directories dated 2026-06-25 through 2026-07-06
- **URL:** <https://github.com/fewtarius/llama-ai/tree/1017f3dfdce3ca2b06aa9007b23295db3bb35722/benchmarks>
- **Supports:** existence of raw-ish response JSON, stats, server logs, summaries, and cold/warm cache-state observations on project-author machines.
- **Limitations:** not independently reproduced; model hashes, full environment, cache-page effects, and experimental controls are incomplete for universal conclusions. Not HaloFPX measurement evidence.

## S14-005 — user-isolation design and implementation map

- **Title/publisher:** `User Isolation Design`, fewtarius/CachyLLama
- **Revision/date:** file at commit `6be7459`, accessed 2026-07-16
- **URL:** <https://github.com/fewtarius/CachyLLama/blob/6be745998f568e379ea197fcf827baec73ff9940/docs/development/user-isolation-design.md>
- **Supports:** intended namespace, API validation, slot affinity, cap, compatibility, and file map.
- **Limitations/conflicts:** design says anonymous requests share the cap; inspected accounting code does not increment the anonymous bucket and explicitly describes global-only throttling. The HTTP fast path inspects `tasks` only after moving that vector into the queue. Release clears slot identity before the counter callback, and prompt-similarity selection lacks a user filter.

## S14-006 — Agent Harness evidence architecture

- **Title/publisher:** local canonical Agent Harness, `guide/architecture.md`
- **Revision/date:** local file accessed 2026-07-16
- **Path:** `C:\Users\britt\Documents\Agent_Harness\guide\architecture.md`
- **Supports:** evidence-to-wiki promotion, candidate/review/published states, and closeout review discipline.
- **Limitations:** governance source, not technical evidence for CachyLLama behavior.

## S14-007 — license texts

- **llama-ai:** [GPL-3.0 text at `1017f3d`](https://github.com/fewtarius/llama-ai/blob/1017f3dfdce3ca2b06aa9007b23295db3bb35722/LICENSE)
- **CachyLLama/llama.cpp:** [MIT text at `6be7459`](https://github.com/fewtarius/CachyLLama/blob/6be745998f568e379ea197fcf827baec73ff9940/LICENSE)
- **Supports:** repository-level license inventory.
- **Limitations:** not legal advice; copied patches, scripts, documentation, benchmark inputs, dependencies, and model assets require per-artifact review in section 16.
