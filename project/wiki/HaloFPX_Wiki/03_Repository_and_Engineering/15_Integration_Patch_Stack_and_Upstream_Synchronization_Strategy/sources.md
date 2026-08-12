---
section_id: "15"
title: "Integration source ledger"
status: "verified"
last_verified: "2026-07-16"
applies_to:
  repositories:
    - "ggml-org/llama.cpp@788e07dc91d266ad3162a1ce9037665656269689"
    - "charlie12345/ROCmFPX@a5605a72768c6562241b248e268e33dc92787394"
    - "fewtarius/CachyLLama@6be745998f568e379ea197fcf827baec73ff9940"
    - "fewtarius/llama-ai@1017f3dfdce3ca2b06aa9007b23295db3bb35722"
  software_versions: ["Git documentation 2.54.0", "GitHub Docs accessed 2026-07-16"]
  hardware_revisions: []
related_sections: ["11", "13", "14", "16"]
---

# Integration source ledger

All Internet sources were accessed 2026-07-16. Repository links are immutable commit or blob URLs. Branch heads may have advanced after the snapshot.

## Repository and source-code evidence

### S15-001 - llama.cpp upstream anchor

- Title/publisher: `ggml-org/llama.cpp`, commit `788e07dc91d266ad3162a1ce9037665656269689`
- URL: https://github.com/ggml-org/llama.cpp/commit/788e07dc91d266ad3162a1ce9037665656269689
- Revision/date: full commit above; committed 2026-07-17 06:42:59Z
- Supports: exact research anchor, upstream repository/default lineage, source tree, MIT license.
- Limitations: a snapshot, not a promise of compatibility with any downstream fork.

### S15-002 - ROCmFPX donor snapshot

- Title/publisher: `charlie12345/ROCmFPX`, commit `a5605a72768c6562241b248e268e33dc92787394`
- URL: https://github.com/charlie12345/ROCmFPX/commit/a5605a72768c6562241b248e268e33dc92787394
- Repository metadata: https://api.github.com/repos/charlie12345/ROCmFPX
- Revision/date: full commit above; committed 2026-07-17 02:34:40Z
- Supports: exact donor tree/head, two-parent integration merge, independent-repository topology, MIT license, ROCmFPX documentation at this revision.
- Limitations: GitHub `fork: false` and absence of a Git merge base do not identify the original file-level upstream snapshot; feature claims still need section 13 validation.

### S15-003 - ROCmFP4 upstream integration record

- Title/publisher: `ROCMFP4-UPSTREAM-INTEGRATION.md`, ROCmFPX
- URL: https://github.com/charlie12345/ROCmFPX/blob/a5605a72768c6562241b248e268e33dc92787394/ROCMFP4-UPSTREAM-INTEGRATION.md
- Revision/date: file at `a5605a72768c6562241b248e268e33dc92787394`; accessed 2026-07-16
- Supports: repository-stated historical baseline `llama.cpp b9438`, commit `22cadc1944f4658214aee03abd08240358840a95`, and upstream-integration workspace intent.
- Limitations: project-authored record; it does not prove current full-tree ancestry or completeness.

### S15-004 - CachyLLama donor and upstream merge

- Title/publisher: `fewtarius/CachyLLama`, commit `6be745998f568e379ea197fcf827baec73ff9940`
- URL: https://github.com/fewtarius/CachyLLama/commit/6be745998f568e379ea197fcf827baec73ff9940
- Repository metadata: https://api.github.com/repos/fewtarius/CachyLLama
- Revision/date: full commit above; committed 2026-07-09 00:17:28Z
- Supports: exact donor tree, MIT license, GitHub fork relationship, parents `c8ead677...` and upstream `92366df3...`, merge-base/divergence calculations.
- Limitations: commit message and repository code are primary implementation evidence, not independent validation of cache correctness or performance.

### S15-005 - llama-ai wrapper, submodule, and license boundary

- Title/publisher: `fewtarius/llama-ai`, commit `1017f3dfdce3ca2b06aa9007b23295db3bb35722`
- URL: https://github.com/fewtarius/llama-ai/tree/1017f3dfdce3ca2b06aa9007b23295db3bb35722
- Revision/date: full commit above; committed 2026-07-09 00:21:33Z
- Supports: CachyLLama gitlink `6be745998f568e379ea197fcf827baec73ff9940`, SSH submodule URL/branch metadata, GPL-3.0-or-later source statement, CC-BY-NC-SA-4.0 documentation statement.
- Limitations: repository claims and bundled benchmarks are not promoted as HaloFPX measurements; file-level license review remains required.

## Git workflow evidence

### S15-006 - git-rebase manual

- Title/publisher: `git-rebase` documentation, Git project
- URL: https://git-scm.com/docs/git-rebase/2.54.0
- Revision/date: manual last updated in Git 2.54.0, 2026-04-20
- Supports: commit replay/rewrite semantics, conflict stop/continue/abort, rebase behavior.
- Limitations: documents mechanics, not the correct HaloFPX policy.

### S15-007 - git-range-diff manual

- Title/publisher: `git-range-diff` documentation, Git project
- URL: https://git-scm.com/docs/git-range-diff/2.54.0
- Revision/date: Git 2.54.0 documentation; accessed 2026-07-16
- Supports: comparing two versions of a patch series and limitations around merge commits.
- Limitations: patch similarity is not runtime equivalence.

### S15-008 - git-rerere manual

- Title/publisher: `git-rerere` documentation, Git project
- URL: https://git-scm.com/docs/git-rerere/2.54.0
- Revision/date: Git 2.54.0 documentation; accessed 2026-07-16
- Supports: recording and reusing manual conflict resolutions, enablement and inspection commands.
- Limitations: reuse may be textually applicable but semantically wrong after upstream change.

### S15-009 - git-cherry-pick manual

- Title/publisher: `git-cherry-pick` documentation, Git project
- URL: https://git-scm.com/docs/git-cherry-pick/2.54.0
- Revision/date: Git 2.54.0 documentation; accessed 2026-07-16
- Supports: applying named commits, `-x` provenance trailer, merge mainline requirement, abort/continue.
- Limitations: clean application does not prove dependencies, licensing, or behavior.

### S15-010 - git-bisect manual

- Title/publisher: `git-bisect` documentation, Git project
- URL: https://git-scm.com/docs/git-bisect/2.54.0
- Revision/date: Git 2.54.0 documentation; accessed 2026-07-16
- Supports: good/bad binary search, automated `bisect run`, skip semantics.
- Limitations: usefulness depends on deterministic tests and buildable intermediate commits.

## Hosted workflow evidence

### S15-011 - protected branches

- Title/publisher: `About protected branches`, GitHub Docs
- URL: https://docs.github.com/en/repositories/configuring-branches-and-merges-in-your-repository/managing-protected-branches/about-protected-branches
- Revision/date: live documentation accessed 2026-07-16
- Supports: required reviews/status checks/conversation resolution/signing/linear history and force-push/deletion controls.
- Limitations: hosted feature availability and configuration may change; exact repository plan/settings are not yet selected.

### S15-012 - syncing a fork

- Title/publisher: `Syncing a fork`, GitHub Docs
- URL: https://docs.github.com/en/pull-requests/collaborating-with-pull-requests/working-with-forks/syncing-a-fork
- Revision/date: live documentation accessed 2026-07-16
- Supports: configure/fetch upstream and merge/fast-forward synchronization; conflict handling.
- Limitations: assumes related fork history; it is not a procedure for reconstructing ROCmFPX ancestry.

## Source conflicts and freshness

- **[VERIFIED]** ROCmFPX describes itself as based on llama.cpp, while GitHub metadata and the pinned Git graph do not encode it as a related fork. Both observations are retained; the file-level lineage is **[OPEN]**.
- **[VERIFIED]** CachyLLama's pinned upstream merge is 125 upstream-only commits behind the pinned llama.cpp head. This is not criticism of the donor; it is a reason to port against an explicit current anchor.
- Recheck all four branch heads, GitHub repository metadata, and Git manual revisions before adopting a new candidate. Never silently replace the pinned source ledger.
