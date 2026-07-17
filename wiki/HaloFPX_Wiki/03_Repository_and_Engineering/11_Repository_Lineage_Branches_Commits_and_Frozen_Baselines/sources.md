---
section_id: "11"
title: "Repository Lineage Sources"
status: "verified"
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

# Sources

Access date for every Internet source: **2026-07-16 America/Los_Angeles**. Dynamic API records are snapshot evidence and must be refreshed for a new baseline.

## Repository and commit sources

### SRC-11-001 - llama-ai repository, branches, and commits

- Publisher/repository: GitHub / `fewtarius/llama-ai`
- URLs: [repository metadata](https://api.github.com/repos/fewtarius/llama-ai), [branches](https://api.github.com/repos/fewtarius/llama-ai/branches?per_page=100), [head commit](https://github.com/fewtarius/llama-ai/commit/1017f3dfdce3ca2b06aa9007b23295db3bb35722), [recent commits at the pinned head](https://api.github.com/repos/fewtarius/llama-ai/commits?sha=1017f3dfdce3ca2b06aa9007b23295db3bb35722&per_page=15)
- Revision/date: `1017f3dfdce3ca2b06aa9007b23295db3bb35722`, committed 2026-07-09 UTC
- Supports: default branch, tip, branch tips, recent development signals, license metadata.
- Limitation: branch and repository metadata are mutable; commit object is stable.

### SRC-11-002 - llama-ai README and submodule declaration

- Publisher/repository: `fewtarius/llama-ai`
- URLs: [README at pinned commit](https://github.com/fewtarius/llama-ai/blob/1017f3dfdce3ca2b06aa9007b23295db3bb35722/README.md), [.gitmodules at pinned commit](https://github.com/fewtarius/llama-ai/blob/1017f3dfdce3ca2b06aa9007b23295db3bb35722/.gitmodules), [tree](https://api.github.com/repos/fewtarius/llama-ai/git/trees/1017f3dfdce3ca2b06aa9007b23295db3bb35722?recursive=1)
- Revision/date: `1017f3dfdce3ca2b06aa9007b23295db3bb35722`
- Supports: operational role, stated CachyLLama relationship, gitlink path/SHA, licenses.
- Limitation: performance and compatibility statements are repository claims, not HaloFPX measurements.

### SRC-11-003 - CachyLLama repository metadata

- Publisher/repository: GitHub / `fewtarius/CachyLLama`
- URLs: [repository metadata](https://api.github.com/repos/fewtarius/CachyLLama), [branches](https://api.github.com/repos/fewtarius/CachyLLama/branches?per_page=100), [tags](https://api.github.com/repos/fewtarius/CachyLLama/tags?per_page=100), [releases](https://api.github.com/repos/fewtarius/CachyLLama/releases?per_page=100)
- Revision/date: observed `master` at `6be745998f568e379ea197fcf827baec73ff9940`
- Supports: GitHub fork parent/source, default branch, dated branches, empty tag/release lists, license metadata.
- Limitation: dynamic metadata.

### SRC-11-004 - CachyLLama pinned head, tree, and README

- Publisher/repository: `fewtarius/CachyLLama`
- URLs: [head commit](https://github.com/fewtarius/CachyLLama/commit/6be745998f568e379ea197fcf827baec73ff9940), [README](https://github.com/fewtarius/CachyLLama/blob/6be745998f568e379ea197fcf827baec73ff9940/README.md), [recursive tree](https://api.github.com/repos/fewtarius/CachyLLama/git/trees/6be745998f568e379ea197fcf827baec73ff9940?recursive=1), [recent commits](https://api.github.com/repos/fewtarius/CachyLLama/commits?sha=6be745998f568e379ea197fcf827baec73ff9940&per_page=15)
- Revision/date: `6be745998f568e379ea197fcf827baec73ff9940`, committed 2026-07-09 UTC
- Supports: upstream merge parents, cache-focused role, active areas, absence of gitlinks.
- Limitation: README benchmark claims were not adopted as measured facts.

### SRC-11-005 - llama.cpp versus CachyLLama comparison

- Publisher/repository: GitHub compare API / `ggml-org/llama.cpp`
- URL: [master compared with fewtarius:master](https://api.github.com/repos/ggml-org/llama.cpp/compare/master...fewtarius:master)
- Revision/date: base observed at `788e07dc91d266ad3162a1ce9037665656269689`; head `6be745998f568e379ea197fcf827baec73ff9940`
- Supports: merge base `92366df...`, 53 ahead, 125 behind, diverged status.
- Limitation: counts change when the named base branch advances; the two full endpoint SHAs make this observation interpretable.

### SRC-11-006 - ROCmFPX repository, branches, tags, and releases

- Publisher/repository: GitHub / `charlie12345/ROCmFPX`
- URLs: [metadata](https://api.github.com/repos/charlie12345/ROCmFPX), [branches](https://api.github.com/repos/charlie12345/ROCmFPX/branches?per_page=100), [tags](https://api.github.com/repos/charlie12345/ROCmFPX/tags?per_page=100), [releases](https://api.github.com/repos/charlie12345/ROCmFPX/releases?per_page=100)
- Revision/date: observed `main` at `a5605a72768c6562241b248e268e33dc92787394`
- Supports: non-fork metadata, branch topology, no tags/releases, MIT metadata.
- Limitation: GitHub “fork” false does not negate code lineage; it only describes the hosting graph.

### SRC-11-007 - ROCmFPX README and notices

- Publisher/repository: `charlie12345/ROCmFPX`
- URLs: [README](https://github.com/charlie12345/ROCmFPX/blob/a5605a72768c6562241b248e268e33dc92787394/README.md), [third-party notices](https://github.com/charlie12345/ROCmFPX/blob/a5605a72768c6562241b248e268e33dc92787394/THIRD_PARTY_NOTICES.md), [recursive tree](https://api.github.com/repos/charlie12345/ROCmFPX/git/trees/a5605a72768c6562241b248e268e33dc92787394?recursive=1)
- Revision/date: `a5605a72768c6562241b248e268e33dc92787394`
- Supports: stated llama.cpp base, experimental status, canonical main branch, ROCmFPX scope, notices, absence of gitlinks.
- Limitation: repository benchmarks and feature claims require independent review/measurement.

### SRC-11-008 - ROCmFP4 upstream integration note

- Publisher/repository: `charlie12345/ROCmFPX`
- URL: [ROCMFP4-UPSTREAM-INTEGRATION.md](https://github.com/charlie12345/ROCmFPX/blob/a5605a72768c6562241b248e268e33dc92787394/ROCMFP4-UPSTREAM-INTEGRATION.md)
- Revision/date: file at `a5605a72768c6562241b248e268e33dc92787394`
- Supports: historical branch `rocmfp4-upstream-b9438-integration` and claimed baseline `b9438`, `22cadc194`.
- Limitation/conflict: tag and commit do not identify the same upstream object; see SRC-11-013.

### SRC-11-009 - ROCmFPX commit graph and recent work

- Publisher/repository: `charlie12345/ROCmFPX`
- URLs: [head commit](https://github.com/charlie12345/ROCmFPX/commit/a5605a72768c6562241b248e268e33dc92787394), [root commit](https://github.com/charlie12345/ROCmFPX/commit/ebee2649a7c4e55d52b486b245232a506bdfe05a), [recent commits](https://api.github.com/repos/charlie12345/ROCmFPX/commits?sha=a5605a72768c6562241b248e268e33dc92787394&per_page=15)
- Revision/date: head committed 2026-07-17T02:34:40Z; local date 2026-07-16
- Supports: independently rooted history, current merge, active areas.
- Limitation: absence of a merge base was also checked with Git against current upstream; content provenance still requires a file/commit audit.

### SRC-11-010 - llama.cpp repository metadata and pinned head

- Publisher/repository: GitHub / `ggml-org/llama.cpp`
- URLs: [metadata](https://api.github.com/repos/ggml-org/llama.cpp), [head commit](https://github.com/ggml-org/llama.cpp/commit/788e07dc91d266ad3162a1ce9037665656269689), [recursive tree](https://api.github.com/repos/ggml-org/llama.cpp/git/trees/788e07dc91d266ad3162a1ce9037665656269689?recursive=1)
- Revision/date: `788e07dc91d266ad3162a1ce9037665656269689`, committed 2026-07-17T06:42:59Z; local date 2026-07-16
- Supports: canonical repository, default branch, head, license, absence of gitlinks.
- Limitation: dynamic repository metadata; commit is stable.

### SRC-11-011 - llama.cpp recent commits

- Publisher/repository: `ggml-org/llama.cpp`
- URL: [15 commits ending at snapshot head](https://api.github.com/repos/ggml-org/llama.cpp/commits?sha=788e07dc91d266ad3162a1ce9037665656269689&per_page=15)
- Revision/date: endpoint pinned by SHA
- Supports: bounded active-development signals.
- Limitation: 15 commits are not a roadmap or complete activity analysis.

### SRC-11-012 - llama.cpp release b10054

- Publisher/repository: GitHub Releases / `ggml-org/llama.cpp`
- URLs: [release](https://github.com/ggml-org/llama.cpp/releases/tag/b10054), [tag ref](https://api.github.com/repos/ggml-org/llama.cpp/git/ref/tags/b10054), [target commit](https://github.com/ggml-org/llama.cpp/commit/ac2557cb24def295888ef47f1a35b401d978c510)
- Revision/date: `b10054`, published 2026-07-17T00:29:02Z; target `ac2557cb...`
- Supports: rolling release example and distance from snapshot head.
- Limitation: “newest” is only true at access time.

### SRC-11-013 - official b9438 ref and adjacent commits

- Publisher/repository: `ggml-org/llama.cpp`
- URLs: [b9438 ref](https://api.github.com/repos/ggml-org/llama.cpp/git/ref/tags/b9438), [tag target d749821](https://github.com/ggml-org/llama.cpp/commit/d749821db3bd587932d1ed57d43626cd552c9909), [claimed commit 22cadc194](https://github.com/ggml-org/llama.cpp/commit/22cadc1944f4658214aee03abd08240358840a95)
- Revision/date: `b9438` target committed 2026-05-30; `22cadc194...` committed 2026-05-31
- Supports: exact contradiction and one-commit ancestry relation.
- Limitation: does not explain the ROCmFPX maintainer’s intent.

## Procedure authorities

### SRC-11-014 - Git submodule documentation

- Publisher: Git project
- URLs: [git-submodule](https://git-scm.com/docs/git-submodule), [gitsubmodules](https://git-scm.com/docs/gitsubmodules)
- Revision/date: official documentation accessed 2026-07-16
- Supports: gitlink semantics, initialization/update/status procedure.
- Limitation: project-specific remote availability and recursive dependency behavior still require testing.

### SRC-11-015 - Git bundle and integrity documentation

- Publisher: Git project
- URLs: [git-bundle](https://git-scm.com/docs/git-bundle), [git-fsck](https://git-scm.com/docs/git-fsck)
- Revision/date: official documentation accessed 2026-07-16
- Supports: offline bundle creation/verification and object integrity checks.
- Limitation: bundles exclude working-tree files, LFS payloads, external dependencies, and model artifacts.

### SRC-11-016 - Git revision, remote, and tag documentation

- Publisher: Git project
- URLs: [git-ls-remote](https://git-scm.com/docs/git-ls-remote), [git-rev-parse](https://git-scm.com/docs/git-rev-parse), [git-tag](https://git-scm.com/docs/git-tag)
- Revision/date: official documentation accessed 2026-07-16
- Supports: resolving default refs, recording exact objects, annotated/signable baseline tags.
- Limitation: a signed tag proves control of a key, not build reproducibility or correctness.
