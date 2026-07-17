---
section_id: "11"
title: "Repository Lineage Facts and Constraints"
status: "needs-machine-validation"
last_verified: "2026-07-16"
applies_to:
  repositories:
    - "fewtarius/llama-ai@1017f3dfdce3ca2b06aa9007b23295db3bb35722"
    - "fewtarius/CachyLLama@6be745998f568e379ea197fcf827baec73ff9940"
    - "charlie12345/ROCmFPX@a5605a72768c6562241b248e268e33dc92787394"
    - "ggml-org/llama.cpp@788e07dc91d266ad3162a1ce9037665656269689"
  software_versions: []
  hardware_revisions: []
related_sections:
  - "13"
  - "14"
  - "15"
  - "16"
---

# Facts and constraints

## Repository identity matrix

| Repository | GitHub relationship | License metadata | Default branch and tip | Tags/releases at observation |
|---|---|---|---|---|
| `fewtarius/llama-ai` | Standalone repository | GPL-3.0 | `main` at `1017f3dfdce3ca2b06aa9007b23295db3bb35722` | No GitHub tags or releases returned |
| `fewtarius/CachyLLama` | Fork of `ggml-org/llama.cpp` | MIT | `master` at `6be745998f568e379ea197fcf827baec73ff9940` | No GitHub tags or releases returned |
| `charlie12345/ROCmFPX` | Standalone GitHub repository; content states llama.cpp base | MIT | `main` at `a5605a72768c6562241b248e268e33dc92787394` | No GitHub tags or releases returned |
| `ggml-org/llama.cpp` | Canonical upstream | MIT | `master` at `788e07dc91d266ad3162a1ce9037665656269689` | Rolling build releases; newest observed release was `b10054` |

**[VERIFIED]** The matrix is a volatile remote-state snapshot accessed on 2026-07-16 local time. The two latest default-branch commits have 2026-07-17 UTC timestamps but occurred on 2026-07-16 in America/Los_Angeles (SRC-11-001, SRC-11-003, SRC-11-006, SRC-11-010, SRC-11-012).

**[VERIFIED]** `llama.cpp` release `b10054` was published at `2026-07-17T00:29:02Z` and its ref resolves to commit `ac2557cb24def295888ef47f1a35b401d978c510`. Snapshot `master` is four commits later. A release label therefore does not identify the observed branch tip (SRC-11-012).

## Branches relevant to this project

### llama-ai

| Branch | Tip | Relationship to default branch |
|---|---|---|
| `main` | `1017f3dfdce3ca2b06aa9007b23295db3bb35722` | Default |
| `20260625` | `aa1ed90dc60af604330ea3f9da4dfb10dcb35c78` | 1 unique commit; 40 commits behind `main` at observation |

**[VERIFIED]** The `main` head commit updates the CachyLLama gitlink after an upstream merge. Recent first-parent work also includes cache opt-out, hardware profiles, build portability, and repeated submodule bumps (SRC-11-001).

### CachyLLama

| Branch | Tip | Meaning at observation |
|---|---|---|
| `master` | `6be745998f568e379ea197fcf827baec73ff9940` | Default/current integration tip |
| `20260708` | `c8ead677a7fe42fb0a67e6e866fb254cc338e9fd` | Historical checkpoint; 62 commits behind `master` |
| `20260704` | `4db9548a8c421da633e1124b4d4110b82306803e` | Historical checkpoint; 107 commits behind `master` |
| `20260630` | `de62cdfbd5def9104a396975b0679eb90fd23f5e` | Dated checkpoint |
| `20260625` | `b56d9c952380f760bb69046352324e5f706b1a90` | Dated checkpoint |

**[VERIFIED]** `6be7459` is a two-parent merge. Its second parent, `92366df30d4eaa4b85139b5fd694360237731b19`, is the upstream side and is also the merge base with snapshot `llama.cpp/master`. Against `788e07d`, GitHub reports CachyLLama 53 commits ahead and 125 behind (SRC-11-004, SRC-11-005).

**[INFERENCE]** Dated branches are rollback/reference checkpoints, not releases: they are unprotected, no releases exist, and their names encode dates. Consumers must still pin full SHAs.

### ROCmFPX

| Branch | Tip | Relationship to `main` |
|---|---|---|
| `main` | `a5605a72768c6562241b248e268e33dc92787394` | Default/canonical per README |
| `experimental-rocmfpx-branch` | `a6a93765f7ce9779c13f9881164a65f7a9f31198` | Ancestor; 112 commits behind |
| `archive/main-before-experimental-2026-07-11` | `5b3956605309dd3e6beed49c8f3a41423ba71d25` | Ancestor; 100 commits behind |
| `qualify/hy3-on-github-main-20260716` | `a8b5fa906ccd13c6a8ca06d55aa287854c376868` | Merged qualification tip; 1 commit behind |
| `agent/promote-experimental-to-main-2026-07-11` | `ccac6e55ec7c0fa8acdcb6e80b1a242e4f7d654e` | Promotion work branch |
| `agent/add-buy-me-a-coffee` | `1b49f4a004b8a2e0183c6cda7a61e7d322ac8d60` | Small feature branch |

**[VERIFIED]** Current `main` merged PR 32 for HY3 MTP, ROCmFP2, and cross-platform server fixes. Recent commits also touch ROCmFPX endpoint semantics, converter support, CPU dispatch, CI probes, disk prompt-cache portability, and strict speculative verification (SRC-11-006, SRC-11-009).

**[VERIFIED]** The repository has an independent root commit `ebee2649a7c4e55d52b486b245232a506bdfe05a` dated 2026-06-19. GitHub fork metadata is false. Although imported upstream commits and files occur in its history, no merge base exists between its current tip and snapshot `llama.cpp/master` (SRC-11-006, SRC-11-009).

**[RECOMMENDATION]** Do not publish a numeric ROCmFPX-versus-upstream ahead/behind value. With unrelated roots, `git rev-list --left-right` has no shared base and a tree diff mixes upstream churn, project changes, generated UI content, and file movement.

### llama.cpp

**[VERIFIED]** `master` is the default branch. Snapshot head `788e07d` adds Vulkan Q2_0 support. The preceding sample includes SYCL and OpenCL fixes, DeepSeekV4 fused operations, tensor-parallel model fixes, recurrent-state tests, server slot save/restore, conversion fixes, and vendor updates (SRC-11-010, SRC-11-011).

**[INFERENCE]** Upstream is a high-churn integration source, not a safe build selector. The observed release cadence and four commits between `b10054` and snapshot `master` demonstrate why a name such as `master` or “latest release” is insufficient for HaloFPX reproduction.

## Submodules and vendored state

**[VERIFIED]** The only gitlink found across the four snapshot trees is `fewtarius/llama-ai:CachyLLama` at `6be745998f568e379ea197fcf827baec73ff9940`. CachyLLama, ROCmFPX, and llama.cpp have an empty root `.gitmodules` file and no mode-`160000` entries (SRC-11-002 through SRC-11-004, SRC-11-006, SRC-11-010).

**[VERIFIED]** An empty `.gitmodules` does not freeze downloaded dependencies, generated WebUI artifacts, model files, ROCm packages, compilers, or vendor archives. Section 16 must lock those independently.

## Historical-baseline contradiction

**[VERIFIED]** ROCmFPX file `ROCMFP4-UPSTREAM-INTEGRATION.md` states “official llama.cpp `b9438`, commit `22cadc194`” as an integration baseline (SRC-11-008).

**[VERIFIED]** Official upstream ref `refs/tags/b9438` resolves to `d749821db3bd587932d1ed57d43626cd552c9909`. Full commit `22cadc1944f4658214aee03abd08240358840a95` is its immediate successor, one commit later (SRC-11-013).

**[OPEN]** It is unknown whether the document intended the build tag, the one-commit-later branch state, or a local build-number convention. Preserve both IDs; do not rewrite history or equate them.

## Active-development signals and limits

| Repository | Source-backed recent signals | Limitation |
|---|---|---|
| llama-ai | Submodule bumps, cache opt-out, profiles, build portability | No release boundary; head last changed July 9 |
| CachyLLama | SSD checkpoint correctness, user/cache behavior, upstream merges | README performance claims are project claims, not HaloFPX measurements |
| ROCmFPX | ROCmFP2/3/4/6/8, HY3/MTP, converter, CPU/HIP/Vulkan/server tests | Experimental APIs and formats can change |
| llama.cpp | Backends, models, server, TP, conversion, tests | A short commit sample is not a complete roadmap |

**[RECOMMENDATION]** Treat commit history as a change signal only. Feature acceptance requires the code/patch inventory in sections 13 and 14 and controlled integration in section 15.
