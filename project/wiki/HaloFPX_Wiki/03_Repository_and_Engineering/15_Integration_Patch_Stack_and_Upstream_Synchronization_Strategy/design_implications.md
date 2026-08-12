---
section_id: "15"
title: "Integration design implications"
status: "needs-machine-validation"
last_verified: "2026-07-16"
applies_to:
  repositories:
    - "HaloFPX integration repository (proposed)"
    - "ggml-org/llama.cpp@788e07dc91d266ad3162a1ce9037665656269689"
    - "charlie12345/ROCmFPX@a5605a72768c6562241b248e268e33dc92787394"
    - "fewtarius/CachyLLama@6be745998f568e379ea197fcf827baec73ff9940"
    - "fewtarius/llama-ai@1017f3dfdce3ca2b06aa9007b23295db3bb35722"
  software_versions: ["Git documentation 2.54.0"]
  hardware_revisions: ["two matched AMD Strix Halo systems; exact revisions open"]
related_sections: ["11", "13", "14", "16"]
---

# Integration design implications

## 15-fork-structure

**[RECOMMENDATION]** Use one canonical HaloFPX integration repository with three read-only donor remotes and one writable origin:

| Remote/ref family | Role | Rewrite policy |
|---|---|---|
| `llama/master` | Authoritative upstream history | Never rewritten locally |
| `rocmfpx/main` | ROCmFPX donor snapshot | Never rewritten locally |
| `cachy/master` | CachyLLama donor snapshot | Never rewritten locally |
| `origin/topic/*` | One owned patch lane or narrowly scoped change | May rebase until published |
| `origin/candidate/<date>-<upstream-shortsha>` | Fixed ordered composition under CI | Never force-pushed after review starts |
| `origin/release/*` plus signed tag | Accepted, reproducible baseline | Immutable |

Use annotated tags such as `donor/rocmfpx-<shortsha>`, `donor/cachy-<shortsha>`, and `baseline/llama-<shortsha>` to make all inputs recoverable. A lock/manifest in the future implementation repository must contain full 40-character SHAs and patch IDs; tags alone are not authority.

## 15-patch-lanes-and-ownership

| Order | Lane | Scope | Default owner role | Gate before next lane |
|---:|---|---|---|---|
| 00 | upstream anchor | Exact llama.cpp baseline and build metadata | integration maintainer | Clean upstream build/test baseline |
| 10 | ROCmFPX | Formats, converters, CPU reference, ROCm/Vulkan kernels and narrow serving hooks | quant/backend owners | Reference vectors plus backend gates from section 13 |
| 20 | Cachy semantics | Selected MIT cache/session semantics, not a wholesale fork merge | cache owner | Persistence, isolation, corruption-as-miss, hybrid-state tests |
| 30 | Halo fabric | Dual-link transport, rank lifecycle, collectives/adapters | transport owner | Link/rank failure and single-node fallback tests |
| 40 | HaloKV | Rank-local storage, manifest, integrity, eviction, ABI | cache owner | Restart, mismatch, corruption, concurrency, recovery tests |
| 50 | product | API, scheduling, configuration, packaging, observability | product owner | End-to-end and upgrade/rollback gates |

**[RECOMMENDATION]** Encode ownership with `CODEOWNERS` plus a patch manifest. Cross-lane changes require each affected owner. Generated changes belong in the same commit as their source or in an immediately adjacent, mechanically reproducible commit.

## 15-rocmfpx-normalization

**[INFERENCE]** Because pinned ROCmFPX and llama.cpp have no Git merge base, `merge`, `rebase`, and ahead/behind counts cannot express their true source delta. An unrelated-history merge would preserve two roots but still would not identify which upstream snapshot individual ROCmFPX files came from.

**[RECOMMENDATION]** Perform a one-time normalization:

1. Select an exact llama.cpp anchor supported by the lineage and patch inventories.
2. Reconstruct lane 10 as small semantic commits on that anchor, preserving donor SHA/path/author/license trailers.
3. Compare every donor-owned path and intentional deletion against ROCmFPX at the pinned SHA.
4. Record excluded files and reasons; absence from the port must not be silent.
5. Validate the reconstructed series independently before stacking cache or product changes.

Do not squash all ROCmFPX behavior into one import commit: that would make regressions and upstreaming opaque. Do not cherry-pick ROCmFPX merge commits: their parent topology is not the HaloFPX topology.

## 15-sync-cadence

**[RECOMMENDATION]** Use a risk-triggered cadence rather than chasing every upstream commit:

- Daily automation: fetch donor refs; report new SHAs, changed paths, security notices, and likely conflict surfaces. No automatic integration.
- Weekly: attempt a disposable upstream replay of each unpublished lane; run `range-diff`, build/unit gates, and record conflicts. Defer release if required hardware is unavailable.
- Monthly: nominate an exact upstream SHA for a candidate after all lane owners review the accumulated delta.
- Immediate: evaluate upstream security/correctness fixes and regressions affecting supported models/backends. Backport a minimal fix when a full sync is unsafe.
- Release: tag only after both matched machines pass the required matrix and rollback artifacts are preserved.

**[RECOMMENDATION]** Maintain a declared support window (for example, current release anchor plus one previous anchor), but choose its duration only after machine validation reveals synchronization cost.

## 15-rebase-merge-and-conflict-policy

- Rebase a private/unpublished `topic/*` lane onto a nominated anchor. Save its old tip and review old versus new with `git range-diff` [S15-006, S15-007].
- Merge validated lanes into a frozen candidate in numeric order using explicit merge commits. The merge boundary records which lane entered the candidate and provides a revert unit.
- Never rebase or force-push a candidate after review begins or any release ref. Corrections create a new candidate.
- On conflict, stop; classify it as textual, API, semantic, generated, or test/fixture. The owning lane resolves it and a second affected owner reviews cross-lane conflicts.
- Never use blanket `-X ours`, `-X theirs`, path replacement, or generated-file overwrite as conflict policy.
- `rerere` may propose a prior resolution, but the staged resolution and tests must be reviewed as new work [S15-008].
- If a resolution changes behavior beyond preserving the lane's prior intent, create a separate adaptation commit with rationale and tests.

## 15-cherry-pick-policy

**[RECOMMENDATION]** Cherry-pick only a single, self-contained, licensed commit when its dependencies and tests are known. Use `git cherry-pick -x`; then add project provenance trailers and preserve authorship [S15-009].

Reject or manually port when the source is a merge commit, bundles unrelated behavior, depends on unselected commits, crosses a license boundary, changes cache ABI without migration/invalidation, or cannot pass its lane gate. A source PR number is useful context but never substitutes for the exact commit.

## 15-upstream-contribution-strategy

**[RECOMMENDATION]** Reduce the permanent delta by upstreaming in this order:

1. Generic bug fixes, tests, and refactors with no Halo dependency to llama.cpp.
2. Independently useful ROCm/Strix Halo backend improvements behind established interfaces.
3. Reusable cache correctness primitives with format/version and corruption behavior documented.
4. Keep dual-link topology policy, rank-local product cache policy, and deployment orchestration downstream unless maintainers request a generic interface.

Each upstream submission should be minimal, based on current upstream, independently tested, and free of dependent Halo-only patches. Human owners remain responsible for the PR description, review responses, and long-term maintenance. Once accepted upstream, replace the downstream patch with the upstream commit in a dedicated drop/rewire change and confirm equivalence with `range-diff` plus tests.

## 15-bisectability-and-failure-boundaries

- Each non-merge patch must state one intent and keep the configured gate buildable.
- Put compatibility shims before their consumers and removals after all consumers migrate.
- Record cache ABI changes explicitly. Old or corrupt cache content must be invalidated, not interpreted optimistically.
- Fabric commits must name rank 0/control-plane ownership, worker ownership, timeout/cancellation behavior, and single-node fallback.
- Candidate merge commits may be tested as units, but lane commits must also be locally bisectable.
- A failed candidate is quarantined; do not patch forward on the release ref or obscure the failing boundary with a squash.

## 15-avoid-permanent-fork

**[RECOMMENDATION]** Track fork cost as a release artifact: downstream commit count, touched upstream paths, unresolved conflict count, oldest non-upstreamed patch, upstreamed/dropped patches, and machine-hours required for the sync. A lane that repeatedly conflicts should be redesigned behind a stable interface, split into a library/process, or proposed upstream. This evidence—not a target commit count—determines whether the fork remains maintainable.
