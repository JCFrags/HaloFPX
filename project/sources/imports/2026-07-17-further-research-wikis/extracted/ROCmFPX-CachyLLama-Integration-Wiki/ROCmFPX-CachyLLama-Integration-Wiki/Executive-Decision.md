---
title: Executive Decision
description: Approved integration posture, scope, and non-goals.
status: Proposed for maintainer approval
evidence-date: 2026-07-17
canonical-repository: charlie12345/ROCmFPX
---

# Executive Decision

> [!NOTE]
> Evidence is pinned to **2026-07-17**. Repository heads and source links are locked in [[Source-Register]] and `evidence/commit-lock.json`.


## Decision

Use **ROCmFPX as the canonical fork**, keep **llama.cpp as the upstream authority**, and consume CachyLLama only through a controlled donor-intake process. The donor branch is not a secondary upstream and must never become a merge parent of `main`.

The integration program is divided into four treatment classes:

| Treatment | Meaning | Approval condition |
|---|---|---|
| **Cherry-pick** | Import an exact donor commit with `-x`, preserving authorship and history. | Exact isolated commit; MIT file/license evidence; no GPL-parent origin; upstream overlap classified; patch applies with only mechanical conflict resolution. |
| **Manual port** | Re-express an MIT donor change against current ROCmFPX architecture, retaining attribution. | Exact donor paths/commits recorded; semantic changes documented; tests demonstrate equivalence; conflicts are architectural rather than licensing-driven. |
| **Adapt behind interface** | Add a canonical abstraction, then implement donor-inspired behavior as one provider. | Existing ROCmFPX behavior remains an adapter; feature is default-off; provider contract is independently tested. |
| **Clean-room reimplementation** | Implement from an approved behavioral specification without copying source. | Required for GPL-parent behavior and used when exact donor provenance is incomplete or copying risk exceeds the value of a source port. |

> [!WARNING]
> The direct-cherry-pick roster is **empty at this evidence point**. Both engine repositories are MIT, but the donor head is a mixed upstream/donor history and the exact introduction commits for the selected features have not been locked. Repository-level license compatibility alone is not sufficient provenance. [S11] [S13] [S28] [S29]

## Selected scope

The program may integrate:

- cross-restart persistent checkpoint storage;
- hot/warm/cold retention policy and bounded disk use;
- safe prefetch/readahead;
- global system-prefix reuse with explicit boundary contracts;
- hybrid attention/recurrent restore semantics;
- tenant-scoped cache routing and scheduling isolation;
- optional slot affinity;
- optional expert activation telemetry;
- clean-room CPU ISA build-preset detection.

## Explicit non-goals

- Merging `fewtarius/CachyLLama/master` into ROCmFPX.
- Replacing ROCmFPX's existing `--cache-disk` behavior in place.
- Treating donor benchmark claims as acceptance evidence.
- Loading donor v3/v1 cache records directly in the server process.
- Copying scripts or code from the GPL-3.0 parent project into the MIT engine tree.
- Publishing a new libllama C ABI before an internal interface proves stable.
- Integrating page-level SSD paging in the first persistent-cache release.

## Why provider-first

ROCmFPX's current cache is integrated into current server and speculative-state logic, including target/draft state pairing and validation. CachyLLama's cache is split into different common/server modules with per-conversation persistent directories and its own formats. A broad port would overwrite canonical safety work and create a single rollback unit spanning server scheduling, state serialization, and disk policy. [S07] [S08] [S10] [S14] [S18] [S28] [S29] [S30]

A provider seam gives three independent rollback choices:

1. Keep the interface and select the current ROCmFPX adapter.
2. Disable persistent writes while retaining validated reads.
3. Compile out the new provider without reverting unrelated server work.

## Approval record

Before implementation begins, maintainers should approve:

- this canonical/upstream/donor role model;
- the cache-format ownership decision;
- the empty initial cherry-pick roster;
- the GPL clean-room boundary;
- the lane dependency graph and acceptance thresholds.


---

**Wiki navigation:** [[Home]] · [[Executive-Decision]] · [[Capability-Decision-Matrix]] · [[Patch-Lanes-and-Dependency-Graph]] · [[Acceptance-Criteria]]
