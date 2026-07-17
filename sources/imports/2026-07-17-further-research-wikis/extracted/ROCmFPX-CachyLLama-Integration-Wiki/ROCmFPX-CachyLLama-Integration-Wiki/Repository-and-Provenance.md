---
title: Repository and Provenance
description: Repository identities, observed heads, licenses, and trust boundaries.
status: Evidence locked
evidence-date: 2026-07-17
canonical-repository: charlie12345/ROCmFPX
---

# Repository and Provenance

> [!NOTE]
> Evidence is pinned to **2026-07-17**. Repository heads and source links are locked in [[Source-Register]] and `evidence/commit-lock.json`.


## Repository roles

| Role | Repository | Observed ref | License evidence | Allowed use |
|---|---|---|---|---|
| Canonical | `charlie12345/ROCmFPX` | `main@a5605a72768c6562241b248e268e33dc92787394` | MIT; third-party notice retained. [S02] [S03] | All integration PRs target this repository. |
| Upstream authority | `ggml-org/llama.cpp` | `master@86d86ed4396b4130922f7b9af26e3d9fc11a591b` | MIT lineage inherited by both engine forks. [S25] | Periodic merge/sync source; upstream-owned fixes should arrive here. |
| Donor engine | `fewtarius/CachyLLama` | `master@6be745998f568e379ea197fcf827baec73ff9940` | MIT and per-file SPDX evidence for reviewed additions. [S13] [S14] [S16] [S17] [S18] | Read-only capability evidence; exact commits may be intake candidates after provenance review. |
| Donor parent | `fewtarius/llama-ai` | `main@1017f3dfdce3ca2b06aa9007b23295db3bb35722` | GPL-3.0. [S23] | Metadata, behavior, and deployment requirements only; no source copying into MIT engine. |

## Requested-name discrepancy

The requested path `llama-ai/CachyLlama` did not resolve as a public GitHub repository. The public CachyLLama engine identifies `fewtarius/llama-ai` as its parent and is published at `fewtarius/CachyLLama`. [S11] [S12] [S24]

This discrepancy is not cosmetic. Every provenance record must state both:

- the user-supplied donor identity; and
- the resolved repository used for evidence.

No implementation intake may begin until a maintainer confirms that `fewtarius/CachyLLama` is the intended donor.

## Provenance confidence levels

| Level | Definition | Permitted action |
|---|---|---|
| **P0 — claim only** | README/design statement, no code path or commit lineage verified. | Requirements gathering only. |
| **P1 — code observed** | Exact path/blob observed with license marker, introduction commit unresolved. | Interface design; no direct cherry-pick. |
| **P2 — lineage established** | Exact introducing commit(s), author, parentage, license, and upstream overlap recorded. | Manual port or cherry-pick review. |
| **P3 — import approved** | P2 plus maintainer/legal approval and green intake tests. | Import into a lane. |

Current selected donor capabilities are generally **P1**. The user-isolation design names five implementation commits but does not provide their SHAs in the reviewed document, so it is not P2. [S19]

## License boundary

The canonical and donor engine `LICENSE` files carry the same MIT terms. The donor parent project carries GPL-3.0 terms. [S02] [S13] [S23]

Consequences:

- Engine files with verified MIT provenance can be ported with attribution.
- Parent scripts and implementation details must not be copied into the MIT engine tree.
- A behavior visible in both parent and engine must be traced to the engine's MIT history before source reuse.
- “Rewriting” a GPL script while looking at its implementation is not automatically clean-room. Use the procedure in [[Licensing-and-Provenance-Gates]].

## Evidence limitations

- This package is a source inspection and integration design, not a runtime validation report.
- Donor performance numbers are donor-reported and are not acceptance baselines. [S12]
- Exact donor feature introduction ranges remain an open provenance task.
- The current heads will move; all future work must use immutable commit IDs, never branch names alone.
- License analysis here is an engineering gate, not legal advice.


---

**Wiki navigation:** [[Home]] · [[Executive-Decision]] · [[Capability-Decision-Matrix]] · [[Patch-Lanes-and-Dependency-Graph]] · [[Acceptance-Criteria]]
