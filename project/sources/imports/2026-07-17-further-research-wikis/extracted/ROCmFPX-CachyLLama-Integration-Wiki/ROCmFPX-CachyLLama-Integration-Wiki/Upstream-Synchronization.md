---
title: Upstream Synchronization
description: Repeatable upstream merge and donor-surveillance procedure.
status: Proposed operating procedure
evidence-date: 2026-07-17
canonical-repository: charlie12345/ROCmFPX
---

# Upstream Synchronization

> [!NOTE]
> Evidence is pinned to **2026-07-17**. Repository heads and source links are locked in [[Source-Register]] and `evidence/commit-lock.json`.


## Authorities

- `upstream/master` is the code synchronization source.
- `donor-cachy/master` is monitored for donor-only capability deltas.
- `donor-parent/main` is monitored only for deployment requirements and submodule identity.
- `origin/main` remains canonical.

## Cadence and triggers

Run a full synchronization at least monthly and additionally when:

- upstream changes server state serialization, MTP/speculative code, memory APIs, or Vulkan/HIP backends;
- donor changes a selected capability path;
- a security or data-corruption fix lands;
- a release candidate is cut.

## Procedure

### 1. Fetch and lock

```bash
git fetch --prune --no-tags origin main
git fetch --prune --no-tags upstream master
git fetch --prune --no-tags donor-cachy master
git fetch --prune --no-tags donor-parent main

CANONICAL=$(git rev-parse origin/main)
UPSTREAM=$(git rev-parse upstream/master)
DONOR=$(git rev-parse donor-cachy/master)
PARENT=$(git rev-parse donor-parent/main)
```

Write those immutable IDs into a new sync record before resolving conflicts.

### 2. Classify donor commits

For each selected path:

```bash
git log --left-right --cherry-pick --oneline       upstream/master...donor-cachy/master -- <path>
```

Classify each donor delta as:

1. **upstream-owned** — exact ancestry or patch equivalent;
2. **donor-original** — not upstream-equivalent and MIT provenance established;
3. **parent-only / GPL-sensitive** — behavior originates in `fewtarius/llama-ai`;
4. **unresolved** — mixed merge or unclear authorship; no import permitted.

Record stable patch IDs for non-merge commits and inspect merge parents for broad donor merges.

### 3. Create frozen sync branch

```bash
git switch --create sync/upstream/YYYYMMDD-${UPSTREAM:0:12} origin/main
git merge --no-ff --no-commit upstream/master
```

Resolve conflicts using [[Conflict-Map]]. Outside ROCmFPX-owned zones, prefer current upstream. Inside owned zones, preserve canonical functionality and port upstream semantics explicitly.

### 4. Validate sync alone

Before rebasing any CachyLLama lane, run:

- canonical feature-off build/test matrix;
- ROCmFPX quantization, HIP/ROCm, Vulkan, MTP, and server cache gates;
- ancestry assertion for known upstream-owned items such as `0bbc87b163ff7826656b1024dac5703e3f2bd6b6`; [S26]
- API/ABI comparison for `include/llama.h` and exported symbols;
- cache-format/state serialization compatibility checks.

### 5. Rebase lanes and range-diff

Rebase private lanes onto the frozen sync result, then review:

```bash
git range-diff <old-base>..<old-lane> <new-base>..<new-lane>
```

Any semantic change in a Critical conflict path invalidates prior lane acceptance and requires re-running it.

### 6. Integration PR

The PR includes:

- `templates/SYNC-RECORD.md` completed;
- exact upstream range and donor observed head;
- conflict list and ownership decisions;
- build/test evidence;
- ABI and cache-format impact statement;
- `AI_CHANGES.md` append. [S04]

## Donor surveillance, not donor synchronization

Never run `git merge donor-cachy/master`. A donor update is converted into one or more capability intake records. Only lane branches based on canonical code may become ancestors of `main`.

## Upstream submission policy

Generic fixes and interfaces that are not ROCmFPX-specific should be proposed upstream where feasible. ROCmFPX-only quantization/backend behavior stays in canonical lanes. A public API such as attention-only memory removal should be discussed upstream before ROCmFPX treats it as stable ABI.

## Sync rollback

If a sync fails acceptance:

- abandon the sync branch; do not partially merge it;
- keep the prior signed release and rollback branch;
- selectively backport only urgent fixes through isolated provenance-reviewed commits;
- record the failed sync and blocking conflicts for the next attempt.


---

**Wiki navigation:** [[Home]] · [[Executive-Decision]] · [[Capability-Decision-Matrix]] · [[Patch-Lanes-and-Dependency-Graph]] · [[Acceptance-Criteria]]
