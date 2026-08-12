# Methodology

## Scope and cutoff

[VERIFIED] Repository branch tips were read at `2026-07-17T18:48:25Z` and frozen in this wiki. “Current” means current at that timestamp, not at the time this package is later opened.

The review covered:

1. repository identity, default branch, status, and immutable tip;
2. top-level and file-level license evidence;
3. ordered parent identity, ancestry, merge bases, and upstream relationship evidence;
4. submodule declarations and observed gitlinks;
5. synchronized or vendored source markers;
6. legally material file-level provenance examples and promoted-branch scopes;
7. build scripts that download or bundle third-party runtime components;
8. official statements from GNU, Creative Commons, Apache, LLVM, GitHub, AMD ROCm, and the U.S. Copyright Office.

## Evidence hierarchy

1. **Immutable repository objects:** commit URLs, blob SHAs, exact file URLs at a commit.
2. **Ordered parent/compare evidence:** `child^1` and `child^2` resolution, ancestry status, merge base, and changed-file scope.
3. **Repository policy files:** `LICENSE`, `.gitmodules`, README, attribution records, synchronization scripts, contribution policy.
4. **Official license and platform statements:** GNU, Creative Commons, Apache, LLVM, GitHub, AMD, U.S. Copyright Office.
5. **Inference:** only after direct evidence, explicitly labeled.

## Provenance vocabulary

| Term | Test used in this review |
|---|---|
| Direct parent | Official compare endpoint resolves `child^N` to an exact base commit; full-clone `git cat-file -p` is the reproducibility check. |
| Graph ancestor | `git merge-base --is-ancestor` equivalent or compare reports the base as merge base and head ahead. |
| Shared/retrievable object | A commit object can be fetched under a repository namespace; this alone does **not** prove current ancestry. |
| Snapshot provenance | A commit message, attribution file, blob identity, or restoration record states that files came from a named upstream revision. |
| Cherry-pick provenance | Commit metadata or attribution records identify a specific upstream commit; `-x` is preferred but not always present. |
| Manual port | Local-authored commit or file is explicitly described as adapted from named upstream commits. |
| Vendored/synchronized source | Source is copied into the tree and tracked by a synchronization marker or vendor script, rather than by gitlink. |
| Submodule | Tree mode `160000` plus `.gitmodules` declaration points to a separate repository commit. |
| Reverse port | Upstream later states that its implementation was ported from the fork. |
| Remote-tracking label | Text such as `upstream/main` names a local Git remote/ref at commit creation time; the commit does not store that remote's URL. |

## Materiality rule for file-level review

[RECOMMENDATION] A file is legally material when it has a non-default license header, comes from a separately named upstream, is generated from third-party material, is bundled into a binary, is copied by a promotion/restore commit, or is a core ROCmFPX-specific implementation without a per-file header.

The provenance ledger is exact for listed files. It intentionally prioritizes legally material paths rather than claiming that every file in the repositories was manually traced.

## Limitations

[UNRESOLVED] The historical URL assigned to ROCmFPX's local remote name `upstream` when commit `2335e6a482b1601d71dff9e860c8feab108c3af2` was created was not recoverable from the commit object. Ordered parents are resolved; remote identity is not.

[UNRESOLVED] GitHub code search is not a complete license scanner. It exposed material exceptions but cannot prove that no additional exception exists. The supplied full-clone scanning procedure is mandatory before release.

[UNRESOLVED] No downloaded TheRock/ROCm tarball was available for component-level inspection. The review therefore treats its assembled runtime as a mixed-license artifact requiring an actual manifest and binary inventory.

[UNRESOLVED] No complete npm installation or prebuilt WebUI tarball was available for transitive license analysis.

[UNRESOLVED] The file-level ledger is a legally material sample, not an assertion that every path in four large repositories has complete line-by-line provenance. The release procedure requires a whole-tree scan and targeted history review.

## Legal interpretation discipline

The matrix separates:

- rights stated by license text;
- technical facts about copying, linking, and process boundaries;
- engineering recommendations; and
- determinations reserved for counsel.

No conclusion that a particular linkage or aggregation is legally a derivative work is presented as a verified fact.
