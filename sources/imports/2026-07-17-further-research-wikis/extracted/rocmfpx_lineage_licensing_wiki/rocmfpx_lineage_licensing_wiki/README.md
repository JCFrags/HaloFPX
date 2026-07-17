# ROCmFPX Repository Lineage and Licensing Wiki

> **Research cutoff:** `2026-07-17T18:48:25Z`  
> **Repositories:** `charlie12345/ROCmFPX`, `fewtarius/llama-ai`, `fewtarius/CachyLLama`, and `ggml-org/llama.cpp`  
> **Purpose:** establish a technically defensible provenance and licensing baseline for a canonical ROCmFPX-derived project.

## Executive determination

[RECOMMENDATION] The lowest-risk canonical architecture is an **MIT-oriented ROCmFPX core** sourced from MIT-granted material in `charlie12345/ROCmFPX`, `fewtarius/CachyLLama`, and/or `ggml-org/llama.cpp`, while retaining every file-specific Apache-2.0, Apache-2.0 WITH LLVM-exception, BSD-2-Clause, MIT-0, public-domain, and other notice. Keep `fewtarius/llama-ai` GPL orchestration code outside that core unless the project intentionally adopts GPL obligations for the affected distributed work.

[VERIFIED] `fewtarius/llama-ai` is not the license baseline for its `CachyLLama` engine submodule. The superproject declares source code GPL-3.0-or-later and documentation CC-BY-NC-SA-4.0, while the `CachyLLama` gitlink resolves to a separately licensed MIT repository.

[VERIFIED] The top-level MIT files in ROCmFPX, CachyLLama, and llama.cpp are **not complete license inventories**. The inspected trees contain file-level Apache-2.0, Apache-2.0 WITH LLVM-exception, BSD-2-Clause, MIT-0/public-domain alternatives, vendored MIT code, model-template notices, and package-manager dependencies.

[VERIFIED] ROCmFPX tip `a5605a72768c6562241b248e268e33dc92787394` is not proven to descend from upstream snapshot commit `5fd2dc2c41c342a75c26f9756ca6b1814ed05fb4`: the compare result reported no common ancestor. ROCmFPX nevertheless records that commit as a source-snapshot revision. Treat it as **content provenance**, not graph ancestry.

[VERIFIED] ROCmFPX merge `2335e6a482b1601d71dff9e860c8feab108c3af2`, titled `Merge remote-tracking branch 'upstream/main'`, has ordered parents `221402af8574faf652b101b6afe225a3f329561f` (first) and `5b3956605309dd3e6beed49c8f3a41423ba71d25` (second). The name `upstream` was a local remote label; Git commit objects do not retain the remote URL. Because the second parent is a ROCmFPX PR-merge commit, this object is **not evidence by itself of a merge from `ggml-org/llama.cpp`**.

[VERIFIED] CachyLLama merge `6be745998f568e379ea197fcf827baec73ff9940` has ordered parents `c8ead677a7fe42fb0a67e6e866fb254cc338e9fd` (first) and `92366df30d4eaa4b85139b5fd694360237731b19` (second). The second parent exists in `ggml-org/llama.cpp`; this is direct upstream graph ancestry.

[VERIFIED] Current upstream llama.cpp later ported Hy3 support back from ROCmFPX in commit `2969d6d15d67a08e7b83f26164b15350c79c5248`, explicitly naming `charlie12345/ROCmFPX` and `src/models/hyv3.cpp`. Provenance therefore runs in both directions.

## Snapshot

| Repository | Default branch | Tip at cutoff | Immediate parent(s) | Top-level declaration | Submodules |
|---|---|---|---|---|---|
| `charlie12345/ROCmFPX` | `main` | `a5605a72768c6562241b248e268e33dc92787394` | `25c71fc6e12d73bb3804127e032d29fb8976ae40`, `a8b5fa906ccd13c6a8ca06d55aa287854c376868` | MIT | `.gitmodules` empty |
| `fewtarius/llama-ai` | `main` | `1017f3dfdce3ca2b06aa9007b23295db3bb35722` | `d8a07baad6ab175f8badbc4d496c9190b0cc3b2d` | GPL-3.0-or-later source; CC-BY-NC-SA-4.0 docs | `CachyLLama` → `6be745998f568e379ea197fcf827baec73ff9940` |
| `fewtarius/CachyLLama` | `master` | `6be745998f568e379ea197fcf827baec73ff9940` | `c8ead677a7fe42fb0a67e6e866fb254cc338e9fd`, `92366df30d4eaa4b85139b5fd694360237731b19` | MIT | `.gitmodules` empty |
| `ggml-org/llama.cpp` | `master` | `86d86ed4396b4130922f7b9af26e3d9fc11a591b` | `7d56da7e546f54fb1fa54ef2bc9ad9a872860ab0` | MIT, with file-level exceptions | `.gitmodules` empty |

## Wiki map

- [Facts and constraints](facts_and_constraints.md) — verified repository, ancestry, license, submodule, vendor, and runtime facts.
- [Repository snapshots](repository_snapshots.md) — immutable commits, ordered merge parents, synchronization markers, and relationship summaries.
- [Provenance map](provenance_map.md) — graph ancestry versus snapshot, cherry-pick, manual-port, vendored, and reverse-port provenance.
- [License compatibility matrix](license_compatibility_matrix.md) — copying, modification, linking, reimplementation, and documentation outcomes.
- [Design implications](design_implications.md) — recommended canonical project boundaries and distribution models.
- [Procedures and checks](procedures_and_checks.md) — repeatable Git, license, SBOM, notice, and clean-room controls.
- [Legal review register](legal_review_register.md) — every review area identified in the inspected scope.
- [Open questions](open_questions.md) — unresolved facts and evidence required to close them.
- [Notices and attribution](notices_and_attribution.md) — notice strategy and current coverage gaps.
- [Methodology](methodology.md) — evidence hierarchy, definitions, and limitations.
- [Sources](sources.md) — primary-source ledger with immutable URLs and access dates.
- [`data/`](data/) — machine-readable repository, ancestry, source, provenance, matrix, question, and review ledgers.
- [`scripts/`](scripts/) — snapshot verification, license scanning, notice coverage, and wiki validation.
- [`templates/`](templates/) — provenance and legal-review records for future contributions.

## Claim labels

| Label | Meaning |
|---|---|
| `[VERIFIED]` | Directly supported by an inspected primary source, immutable Git object, repository file, compare result, or official license statement. |
| `[INFERENCE]` | Reasoned conclusion from verified facts; not itself directly stated by a source. |
| `[RECOMMENDATION]` | Proposed engineering, compliance, or governance control. |
| `[UNRESOLVED]` | Material fact not established from the inspected evidence. |
| `[LEGAL-REVIEW]` | Decision requires qualified counsel or a rights-holder clarification before release. |

## Legal scope

This package is a technical provenance and license-compatibility analysis, not legal advice. Copyright scope, derivative-work status, linking, patent exposure, trademark use, contributor authority, export controls, and license interpretation can depend on jurisdiction and release facts. Items marked `[LEGAL-REVIEW]` are release gates, not optional cleanup.

## Important completeness boundary

[VERIFIED] The package records exact evidence for the listed commits, ordered ancestry edges, and material files.  
[UNRESOLVED] It is not a substitute for a full local clone, complete object-graph inspection, line-by-line history review, binary SBOM, or counsel review of every shipped artifact. Run the supplied procedures against the exact release candidate before publication.
