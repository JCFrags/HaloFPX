# Scope and baselines

> **Audit snapshot:** fork `a5605a72768c6562241b248e268e33dc92787394`; upstream `86d86ed4396b4130922f7b9af26e3d9fc11a591b`; inventory date `2026-07-17`.  
> **Decision vocabulary:** **RETAIN** = capability/ABI must survive; **REFRESH** = preserve capability but re-port onto current upstream; **RETIRE** = remove the fork patch and use upstream or archive it. [S-FORK-HEAD](https://github.com/charlie12345/ROCmFPX/tree/a5605a72768c6562241b248e268e33dc92787394) [S-UP-HEAD](https://github.com/ggml-org/llama.cpp/tree/86d86ed4396b4130922f7b9af26e3d9fc11a591b)

## Audited objects

| Object | Pinned reference | Role | Primary source |
|---|---|---|---|
| ROCmFPX | `a5605a72768c6562241b248e268e33dc92787394` | Audited fork head; merge of PR #32. | [S-FORK-HEAD](https://github.com/charlie12345/ROCmFPX/tree/a5605a72768c6562241b248e268e33dc92787394) [S-C-PR32-M](https://github.com/charlie12345/ROCmFPX/commit/a5605a72768c6562241b248e268e33dc92787394) |
| llama.cpp | `86d86ed4396b4130922f7b9af26e3d9fc11a591b` | Replacement/reference upstream snapshot. | [S-UP-HEAD](https://github.com/ggml-org/llama.cpp/tree/86d86ed4396b4130922f7b9af26e3d9fc11a591b) |
| Historical ROCmFP4 integration point | upstream build `b9438`, commit `22cadc194` | Earliest explicit base documented by the fork. | [S-BASELINE](https://github.com/charlie12345/ROCmFPX/blob/a5605a72768c6562241b248e268e33dc92787394/ROCMFP4-UPSTREAM-INTEGRATION.md) |
| Exact recent comparison base | `5b3956605309dd3e6beed49c8f3a41423ba71d25` | Base of the 100-node post-baseline range. | [S-COMPARE](https://github.com/charlie12345/ROCmFPX/compare/5b3956605309dd3e6beed49c8f3a41423ba71d25...a5605a72768c6562241b248e268e33dc92787394) [S-PR27](https://github.com/charlie12345/ROCmFPX/pull/27) |

The repositories are not connected as a normal GitHub fork with a usable current common-ancestor compare. This inventory therefore uses four primary-source mechanisms: the fork’s explicit integration note, the exact post-baseline ancestry range, complete PR changed-file lists, and semantic comparison of pinned fork/upstream files. [S-BASELINE](https://github.com/charlie12345/ROCmFPX/blob/a5605a72768c6562241b248e268e33dc92787394/ROCMFP4-UPSTREAM-INTEGRATION.md) [S-COMPARE](https://github.com/charlie12345/ROCmFPX/compare/5b3956605309dd3e6beed49c8f3a41423ba71d25...a5605a72768c6562241b248e268e33dc92787394) [S-PR27-FILES](https://github.com/charlie12345/ROCmFPX/pull/27/files) [S-PR32-FILES](https://github.com/charlie12345/ROCmFPX/pull/32/files) [S-FORK-HEAD](https://github.com/charlie12345/ROCmFPX/tree/a5605a72768c6562241b248e268e33dc92787394) [S-UP-HEAD](https://github.com/ggml-org/llama.cpp/tree/86d86ed4396b4130922f7b9af26e3d9fc11a591b)

## Exact 100-node patch-series accounting

The range `5b3956605309dd3e6beed49c8f3a41423ba71d25...a5605a72768c6562241b248e268e33dc92787394` contains 96 authored commits in four PRs plus four merge commits, totaling 100 commit-graph nodes. The series decomposition below is exact at PR boundary level. [S-COMPARE](https://github.com/charlie12345/ROCmFPX/compare/5b3956605309dd3e6beed49c8f3a41423ba71d25...a5605a72768c6562241b248e268e33dc92787394) [S-PR27](https://github.com/charlie12345/ROCmFPX/pull/27) [S-PR28](https://github.com/charlie12345/ROCmFPX/pull/28) [S-PR31](https://github.com/charlie12345/ROCmFPX/pull/31) [S-PR32](https://github.com/charlie12345/ROCmFPX/pull/32)

| Series | Authored commits | Merge nodes | Range nodes | Changed files | Purpose | Source |
| :--- | ---: | ---: | ---: | ---: | :--- | :--- |
| PR #27 | 53 | 1 | 54 | 237 | Promote the validated experimental ROCmFPX branch | [S-PR27](https://github.com/charlie12345/ROCmFPX/pull/27) |
| PR #28 | 1 | 1 | 2 | 1 | Add funding metadata | [S-PR28](https://github.com/charlie12345/ROCmFPX/pull/28) |
| PR #31 | 1 | 1 | 2 | 2 | Honor per-request speculative limits | [S-PR31](https://github.com/charlie12345/ROCmFPX/pull/31) |
| PR #32 | 41 | 1 | 42 | 73 | Integrate HY3 MTP, ROCmFP2, and cross-platform server fixes | [S-PR32](https://github.com/charlie12345/ROCmFPX/pull/32) |

**Check:** `54 + 2 + 2 + 42 = 100`. The canonical per-commit lists remain the PR `commits` views; this wiki inventories every series boundary and every changed path, then names the capability-owning anchor commits separately. [S-PR27-COMMITS](https://github.com/charlie12345/ROCmFPX/pull/27/commits) [S-PR32-COMMITS](https://github.com/charlie12345/ROCmFPX/pull/32/commits) [S-COMPARE](https://github.com/charlie12345/ROCmFPX/compare/5b3956605309dd3e6beed49c8f3a41423ba71d25...a5605a72768c6562241b248e268e33dc92787394)

## File-level scope

The union of the exact PR changed-file lists is **276 unique paths**: 237 in PR #27, one in PR #28, two in PR #31, and 73 in PR #32, with overlap between series. The downloadable [`data/file-map.csv`](data/file-map.csv) records PR membership, subsystem, classification, and rationale for every path. [S-PR27-FILES](https://github.com/charlie12345/ROCmFPX/pull/27/files) [S-PR28](https://github.com/charlie12345/ROCmFPX/pull/28) [S-PR31-FILES](https://github.com/charlie12345/ROCmFPX/pull/31/files) [S-PR32-FILES](https://github.com/charlie12345/ROCmFPX/pull/32/files)

## Classification method

- **RETAIN** means the format ABI, behavior, test oracle, or tuning/validation capability must remain available after migration.
- **REFRESH** means the capability should remain, but the implementation must begin with the pinned current upstream file and re-port only the smallest ROCmFPX-specific hunk set.
- **RETIRE** means the patch is generic upstream code, superseded by current upstream, unrelated to the technical product, or an obsolete parallel proposal.

These are engineering assessments, not claims made by the repositories. Every assessment row includes the primary code/commit evidence from which it was derived.

## Hunk-level caution

A file-level `REFRESH` label is intentionally conservative. Shared files such as `ggml/src/ggml-cuda/mmvq.cu`, `ggml/src/ggml-vulkan/ggml-vulkan.cpp`, `common/speculative.cpp`, and `tools/server/server-context.cpp` contain both fork-owned behavior and generic copied upstream code; they cannot be safely retained or retired wholesale. [S-MMVQ](https://github.com/charlie12345/ROCmFPX/blob/a5605a72768c6562241b248e268e33dc92787394/ggml/src/ggml-cuda/mmvq.cu) [S-VULKAN](https://github.com/charlie12345/ROCmFPX/blob/a5605a72768c6562241b248e268e33dc92787394/ggml/src/ggml-vulkan/ggml-vulkan.cpp) [S-SPEC-FORK](https://github.com/charlie12345/ROCmFPX/blob/a5605a72768c6562241b248e268e33dc92787394/common/speculative.cpp) [S-SERVER-CTX](https://github.com/charlie12345/ROCmFPX/blob/a5605a72768c6562241b248e268e33dc92787394/tools/server/server-context.cpp) [S-UP-HEAD](https://github.com/ggml-org/llama.cpp/tree/86d86ed4396b4130922f7b9af26e3d9fc11a591b)

## Evidence standard

Claims cite pinned source files, commits, PR diffs, or official AMD documentation. Benchmark statements are treated as repository-reported evidence unless accompanied by raw measurements; migration decisions require rerunning the fork’s tests on the target upstream/toolchain. [S-README-FORK](https://github.com/charlie12345/ROCmFPX/blob/a5605a72768c6562241b248e268e33dc92787394/README.md) [S-ROCMFPX-CI](https://github.com/charlie12345/ROCmFPX/blob/a5605a72768c6562241b248e268e33dc92787394/.github/workflows/check-rocmfpx.yml) [S-SWEEP-BACKEND](https://github.com/charlie12345/ROCmFPX/blob/a5605a72768c6562241b248e268e33dc92787394/scripts/sweep-rocmfpx-backend-ops.sh) [S-AMD-STRIX](https://rocm.docs.amd.com/en/docs-7.2.0/how-to/system-optimization/strixhalo.html)
