---
section_id: "11"
title: "Repository Lineage Open Questions"
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

# Open questions

## Priority queue

| ID | Question | Evidence needed | Resolution owner |
|---|---|---|---|
| OQ-11-01 | Which exact ROCmFPX commit and local patch state should become HaloFPX baseline 1? | Inventory both nodes and any workstation clones; compare commits, remotes, worktrees, patches, and successful build receipts | Sections 11, 13, 16 |
| OQ-11-02 | Did ROCmFPX intend upstream tag `b9438`, commit `22cadc1944f...`, or both? | Maintainer clarification or a preserved integration receipt/build log showing the checkout | Section 11 |
| OQ-11-03 | Which CachyLLama commits are required for persistent cache, user isolation, recurrent-state safety, and agent serving? | Commit-to-symbol dependency map and focused tests; compare upstream equivalents | Sections 14, 15 |
| OQ-11-04 | What upstream llama.cpp cadence is supportable: scheduled batch, capability-driven, or security-only? | Integration effort measurements across at least two sync rehearsals | Section 15 |
| OQ-11-05 | Are any local changes or source archives absent from the four GitHub repositories? | Preservation-first filesystem inventory on both nodes and workstation | Sections 11, 16 |
| OQ-11-06 | Which generated or downloaded assets are required for offline builds? | Trace clean builds with network disabled after dependency capture | Section 16 |
| OQ-11-07 | Will any GPL llama-ai code be incorporated, or only used as behavioral reference? | File-level provenance plan and licensing review | Sections 14, 16 |
| OQ-11-08 | What signing authority and key-retention policy will protect baseline tags/manifests? | Project decision and recovery test | Section 16 |
| OQ-11-09 | How will cache ABI/schema compatibility be versioned across source baselines? | Cache-format inventory, corruption tests, upgrade/rollback design | Cache and integration sections |
| OQ-11-10 | What constitutes supported single-node fallback for each dual-node baseline? | Distributed architecture decision plus matched smoke tests | Distributed execution sections |

All items remain **[OPEN]**. No missing answer is silently converted into a project requirement.

## Internet/source-code follow-up

1. **[OPEN]** Monitor the four default branches and releases, but append observations rather than editing the 2026-07-16 snapshot in place.
2. **[OPEN]** Build a precise commit dependency graph for the CachyLLama fork-only range after merge base `92366df30d4eaa4b85139b5fd694360237731b19`.
3. **[OPEN]** Locate upstream equivalents or superseding changes for each selected cache/server patch.
4. **[OPEN]** Audit ROCmFPX commits for copied/cherry-picked upstream provenance and identify the exact tree state represented by its initial independent-root snapshot.
5. **[OPEN]** Check official issues/PRs for format/API stability before freezing ROCmFPX interfaces.
6. **[OPEN]** Confirm release/tag immutability expectations and signature coverage for each donor repository.

## On-machine validation tasks

1. **[OPEN]** Record `git remote -v`, `git status --porcelain=v2 --branch`, `git rev-parse HEAD`, `git submodule status --recursive`, and `git worktree list --porcelain` for every relevant checkout.
2. **[OPEN]** Preserve untracked patches, build scripts, and source archives before normalizing any clone.
3. **[OPEN]** Create and SHA-256-hash full bundles, then prove offline restoration on a clean path.
4. **[OPEN]** Compare both Strix Halo nodes for identical source, toolchain, dependency lock, binary hashes, and runtime configuration.
5. **[OPEN]** Run source/build smoke tests on detached baseline worktrees. Record failures without changing the frozen source.
6. **[OPEN]** Corrupt a disposable cache artifact and prove the runtime rejects it as a miss/recompute; preserve raw logs. Do not perform this test on production cache data.

## Decisions contingent on evidence

- **[OPEN]** Initial product base and patch-stack contents.
- **[OPEN]** Whether to retain ROCmFPX’s history as a vendor import, graft a provenance map, or reconstruct a new integration history. Do not rewrite the donor repository.
- **[OPEN]** Whether llama-ai is packaged, referenced, or partially reimplemented.
- **[OPEN]** Baseline naming, signing keys, retention period, and archive locations.
- **[OPEN]** Synchronization and deprecation policy for old baselines.

## Closure conditions

This section can move from `needs-machine-validation` to `verified` when:

- a first baseline manifest is committed and tagged;
- all source objects and recursive dependencies are available from verified offline archives;
- the two-node source/build identity check has a preserved receipt;
- OQ-11-01, OQ-11-05, OQ-11-06, and OQ-11-08 are resolved or explicitly deferred by a decision record;
- the `b9438` contradiction is clarified or permanently documented as unresolved provenance.
