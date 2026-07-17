---
section_id: "14"
title: "llama-ai and CachyLLama Feature and Patch Inventory"
status: "needs-machine-validation"
last_verified: "2026-07-16"
applies_to:
  repositories:
    - "fewtarius/llama-ai@1017f3dfdce3ca2b06aa9007b23295db3bb35722"
    - "fewtarius/CachyLLama@6be745998f568e379ea197fcf827baec73ff9940"
    - "ggml-org/llama.cpp@92366df30d4eaa4b85139b5fd694360237731b19"
  software_versions: []
  hardware_revisions:
    - "AMD Strix Halo; exact HaloFPX machines not yet measured"
related_sections: ["11", "12", "13", "15", "16"]
---

# llama-ai and CachyLLama Feature and Patch Inventory

## Decision-useful summary

**[VERIFIED]** At the pinned baseline, `llama-ai` is an operational wrapper and evidence repository: it pins CachyLLama as a submodule, selects tier-aware model profiles, builds Vulkan/ROCm/Metal variants, starts `llama-server`, and contains a cold/warm benchmark harness. The cache, server, recurrent-state, user-isolation, and expert-tracking changes live primarily in CachyLLama. [S14-001][S14-002]

**[VERIFIED]** CachyLLama commit `6be7459` merges upstream llama.cpp commit `92366df` and retains a 56-file fork delta (about 9,021 additions and 400 deletions). The largest custom surfaces are the SSD cache, system-prompt cache, page manager, server lifecycle, hybrid-memory handling, user isolation, and telemetry. [S14-002][S14-003]

**[RECOMMENDATION]** HaloFPX should selectively port bounded capabilities, not merge this fork wholesale. Begin with a compatibility-stamped, rank-local checkpoint adapter and corruption-as-miss tests. Treat fuzzy continuation, cross-conversation system-prompt reuse, MTP/recurrent restoration, and per-user scheduling as later gates. See [design implications](design_implications.md#recommended-port-boundary).

**[OPEN]** No evidence in these repositories proves correctness or performance on the intended two-node, dual-link HaloFPX topology. The checked-in benchmarks are repository-authored, single-host observations and must not be promoted to HaloFPX measurements. [S14-004]

## Baseline and lineage

| Component | Exact pin | Commit date | Role |
|---|---|---:|---|
| `fewtarius/llama-ai` | `1017f3dfdce3ca2b06aa9007b23295db3bb35722` | 2026-07-08 | runner, profiles, build scripts, service file, benchmarks |
| `fewtarius/CachyLLama` | `6be745998f568e379ea197fcf827baec73ff9940` | 2026-07-08 | llama.cpp fork containing engine/server changes |
| merged upstream parent | `ggml-org/llama.cpp@92366df30d4eaa4b85139b5fd694360237731b19` | 2026-07-08 | correct comparison baseline for the pinned fork |

The `llama-ai` gitlink is exactly `6be7459`; do not substitute the moving CachyLLama branch. The pinned CachyLLama merge has two parents: custom line `c8ead67` and upstream `92366df`. [S14-001][S14-002][S14-003]

## Retrieval map

- [Facts and constraints](facts_and_constraints.md) — implementation and patch inventory.
- [Design implications](design_implications.md) — what HaloFPX should reuse, adapt, or reject.
- [Procedures and checks](procedures_and_checks.md) — reproducible source and two-machine validation.
- [Open questions](open_questions.md) — unresolved correctness, security, and performance gates.
- [Sources](sources.md) — exact pins, primary links, and limitations.

## Research split

1. **Internet/source-code research completed now:** exact pins, upstream merge parent, fork delta, APIs, cache format, matching, profiles, scripts, tests, and internal contradictions.
2. **Required on-machine work:** build the selected HaloFPX base; run deterministic transformer, recurrent, and MTP restart tests; inject torn/corrupt files; test concurrent tenants; measure NVMe/RAM/TTFT and rank-local behavior on both machines.
3. **Contingent decisions:** cache ABI, matching policy, durability mode, permitted sharing scope, eviction budgets, rank ownership, and whether any expert telemetry or runner profile is retained.

## Authority and review state

This is a candidate wiki synthesis following the Agent Harness `sources -> wiki -> decision` boundary. Repository code supports **[VERIFIED]** claims; repository benchmarks remain scoped evidence; HaloFPX adoption remains **[RECOMMENDATION]** or **[OPEN]** until procedures pass. [S14-006]

