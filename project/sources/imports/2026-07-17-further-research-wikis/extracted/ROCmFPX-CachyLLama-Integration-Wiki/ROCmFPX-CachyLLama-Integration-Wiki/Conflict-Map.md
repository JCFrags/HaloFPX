---
title: Conflict Map
description: Path-level collision forecast and deterministic resolution ownership.
status: Proposed
evidence-date: 2026-07-17
canonical-repository: charlie12345/ROCmFPX
---

# Conflict Map

> [!NOTE]
> Evidence is pinned to **2026-07-17**. Repository heads and source links are locked in [[Source-Register]] and `evidence/commit-lock.json`.


## Resolution principle

Conflicts are resolved **toward current ROCmFPX architecture first**, then donor behavior is re-applied through the owning lane. A textual merge that preserves donor code but bypasses canonical speculative-state or disk-safety logic is considered a failed resolution.

## Hot-path map

| Path / area | Severity | Canonical ownership | Donor pressure | Resolution rule | Owning lane |
|---|---:|---|---|---|---|
| `tools/server/server-task.h/.cpp` | **Critical** | Current RAM/per-run disk cache structures, atomic save/load, target/draft/spec handling. [S07] [S08] | Donor task identity and separate persistent cache integration. | Keep canonical types and semantics; extract an interface around them. Never replace file wholesale. | L02–L06, L09 |
| `tools/server/server-context.cpp` | **Critical** | Slot lifecycle, current prompt cache, MTP/spec validation, diffusion additions. [S10] | Persistent restore, allocator affinity, user caps, checkpoint search. | Resolve by behavior slices in separate commits; no mixed cache+scheduler commit. | L05, L08–L11 |
| `common/speculative.*` | **Critical** | Canonical target/draft/spec state and MTP boundary behavior. | Donor hybrid state serialization/restore. | Define capability API and golden boundary tests before any port. | L08 |
| `include/llama.h` and libllama state API | **Critical** | Public ABI at canonical head lacks donor attention-only API. [S21] | Donor adds `llama_memory_seq_rm_attn_only` and expert APIs. [S20] [S22] | Internal interface first; public ABI only in an isolated, upstream-reviewed commit. | L08, L12 |
| `src/llama-memory*`, recurrent/KV implementations | **Critical** | Architecture-specific memory semantics. | Attention-only removal and recurrent-state restore. | Manual semantic port with architecture matrix; reject generic fallback that clears recurrent state. | L08 |
| `common/arg.cpp`, `common/common.h` | **High** | Existing `--cache-disk*`, ROCmFPX/model/server options. | Donor `--cache-ssd*`, user cap, system-cache flags. | Add new names without repurposing existing flags; aliases only in L15. | L02, L09–L10, L15 |
| `common/CMakeLists.txt` | **High** | Canonical common target composition. | Donor adds cache modules. [S28] | Add provider source under explicit compile option; avoid unconditional donor file list. | L02–L05 |
| `tools/server/CMakeLists.txt` | **High** | Canonical server composition includes diffusion and lacks donor page-manager modules. [S30] | Donor adds page manager/SSD cache modules. [S29] | Preserve canonical target; add only lane-owned sources. Do not copy donor CMake wholesale. | L02–L05 |
| `tools/server/server-chat.cpp` | **High** | Request normalization and API compatibility. | `metadata.user_id` / `llama_user_id` extraction. | Parse once into neutral scope input; validation in L09; no cache lookup here. | L09 |
| `tools/server/server-queue.*` | **High** | Existing queue synchronization. | Per-user counters and authoritative cap check. | Define lock order and counter invariant; separate from affinity. | L10 |
| `tools/server/server-common.*` | **High** | Error taxonomy/HTTP mapping. | Rate-limit error/HTTP 429. | Add a narrow error type commit after scheduler behavior exists. | L10 |
| Cache tests under `tools/server/tests` | **High** | Existing per-run cache expectations and Windows cleanup. [S09] | Persistent restart, scope, hybrid tests. | Keep old tests unchanged; add a new persistent test module and shared fixture helpers only when neutral. | L01, L03–L11 |
| `ggml/src/ggml-vulkan/ggml-vulkan.cpp` | **Medium** | Current upstream-derived AMD submission tuning already present. [S26] | Donor advertises APU tuning. | No donor port; assert upstream commit ancestry during sync. | L14 |
| Build/runner scripts | **Medium / license-sensitive** | ROCmFPX-specific scripts and preflight. [S04] | GPL parent CPU/runner behavior. | Clean-room implementation from requirements and black-box tests. | L13 |
| `README.md`, server docs | **Medium** | Canonical product/serving guidance. | Donor flags and benchmark claims. | Document canonical names and independently measured results only. | Every lane final commit |

## Semantic conflict checklist

A conflict is not resolved until the reviewer answers:

- Does a failed load leave **all** target/draft/spec/recurrent state cold and consistent?
- Is the existing per-run cache still selected by the old flags?
- Can an explicit tenant ever search an anonymous or other-tenant namespace?
- Does context shifting update every required speculative/recurrent boundary?
- Is a cache write committed atomically as one logical entry?
- Does feature-off preserve canonical allocation, logging, and request behavior?
- Is the path owned by exactly one lane in the current PR?

## Conflict freeze rule

If an upstream synchronization touches any **Critical** row while a dependent lane is open, freeze that lane, update its base, run `git range-diff`, and repeat its acceptance tests. Do not stack ad hoc conflict-fix commits onto a stale integration base.


---

**Wiki navigation:** [[Home]] · [[Executive-Decision]] · [[Capability-Decision-Matrix]] · [[Patch-Lanes-and-Dependency-Graph]] · [[Acceptance-Criteria]]
