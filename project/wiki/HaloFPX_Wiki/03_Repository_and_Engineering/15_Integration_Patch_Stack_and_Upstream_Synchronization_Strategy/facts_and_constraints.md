---
section_id: "15"
title: "Integration facts and constraints"
status: "needs-machine-validation"
last_verified: "2026-07-16"
applies_to:
  repositories:
    - "ggml-org/llama.cpp@788e07dc91d266ad3162a1ce9037665656269689"
    - "charlie12345/ROCmFPX@a5605a72768c6562241b248e268e33dc92787394"
    - "fewtarius/CachyLLama@6be745998f568e379ea197fcf827baec73ff9940"
    - "fewtarius/llama-ai@1017f3dfdce3ca2b06aa9007b23295db3bb35722"
  software_versions: ["Git documentation 2.54.0"]
  hardware_revisions: ["two matched AMD Strix Halo systems; exact revisions open"]
related_sections: ["11", "13", "14", "16"]
---

# Integration facts and constraints

## 15-current-source-snapshot

Observed 2026-07-16 (the UTC commit timestamps for the two newest heads fall on 2026-07-17):

| Source | Exact revision | Commit time | Relationship and consequence |
|---|---|---|---|
| `ggml-org/llama.cpp` | `788e07dc91d266ad3162a1ce9037665656269689` | 2026-07-17 06:42:59Z | **[VERIFIED]** Default branch `master`; MIT upstream anchor [S15-001]. |
| `charlie12345/ROCmFPX` | `a5605a72768c6562241b248e268e33dc92787394` | 2026-07-17 02:34:40Z | **[VERIFIED]** Default branch `main`; MIT; GitHub reports it is not a fork. A full-history `git merge-base` with S15-001 returned no commit [S15-002]. |
| `fewtarius/CachyLLama` | `6be745998f568e379ea197fcf827baec73ff9940` | 2026-07-09 00:17:28Z | **[VERIFIED]** GitHub fork of llama.cpp; MIT. Its head merges upstream commit `92366df30d4eaa4b85139b5fd694360237731b19` as second parent [S15-004]. |
| `fewtarius/llama-ai` | `1017f3dfdce3ca2b06aa9007b23295db3bb35722` | 2026-07-09 00:21:33Z | **[VERIFIED]** GPL-3.0-or-later source; its `CachyLLama` gitlink is exactly `6be745998f568e379ea197fcf827baec73ff9940` [S15-005]. |

**[VERIFIED]** At the pinned endpoints, CachyLLama and llama.cpp have merge base `92366df30d4eaa4b85139b5fd694360237731b19`; `git rev-list --left-right --count Cachy...llama` reports 53 Cachy-only and 125 upstream-only commits. These counts describe only the pinned graph, not continuing branch state [S15-001, S15-004].

**[VERIFIED]** ROCmFPX documentation names official llama.cpp build `b9438`, commit `22cadc1944f4658214aee03abd08240358840a95`, as an earlier ROCmFP4 integration baseline. That source statement does not create Git ancestry between the current repositories [S15-003].

## 15-git-behavior

- **[VERIFIED]** Rebase replays selected commits onto a different base and rewrites their identities. The Git manual provides abort/continue paths and warns about recovering from upstream rebases [S15-006].
- **[VERIFIED]** `git range-diff` compares two versions of a patch series while ignoring merge commits; it is appropriate for reviewing a rebased topic, not proof of runtime equivalence [S15-007].
- **[VERIFIED]** `git rerere` records a manual conflict resolution and can reuse it when the same conflict recurs; it must be enabled and does not resolve semantic correctness [S15-008].
- **[VERIFIED]** `git cherry-pick -x` appends the source commit identity for conflict-free cherry-picks. Cherry-picking a merge requires an explicit mainline parent [S15-009].
- **[VERIFIED]** `git bisect` searches a commit range using known good and bad endpoints and can automate classification with `git bisect run` [S15-010].
- **[VERIFIED]** GitHub protected branches can require pull-request review, status checks, resolved conversations, signed commits, linear history, and restrictions on force pushes/deletion [S15-011].
- **[VERIFIED]** GitHub's fork synchronization procedure assumes a configured upstream remote and merges or fast-forwards upstream history; conflicts require resolution [S15-012]. That procedure does not cure unrelated histories.

## 15-license-and-provenance-boundary

- **[VERIFIED]** llama.cpp, ROCmFPX, and CachyLLama carry the MIT license at the pinned commits [S15-001, S15-002, S15-004].
- **[VERIFIED]** llama-ai states GPL-3.0-or-later for source code and CC-BY-NC-SA-4.0 for documentation [S15-005].
- **[RECOMMENDATION]** Treat llama-ai as a requirements, deployment, and provenance source until section 16 completes file-level license review. Do not copy its GPL source into an intended MIT core by assumption.
- **[RECOMMENDATION]** Every imported or rewritten patch must record donor repository, full source SHA, original path(s), author attribution, license, dependency patch IDs, and whether it is unchanged, adapted, or clean-room reimplemented.

## 15-project-constraints

- **[ASSUMPTION]** The product source base is intended to retain ROCmFPX behavior while tracking llama.cpp; no canonical HaloFPX implementation repository exists in this section's evidence.
- **[ASSUMPTION]** Persistent cache is rank-local. Shared cache files across ranks are forbidden unless a later protocol proves atomic ownership and compatibility.
- **[RECOMMENDATION]** A cache artifact is accepted only when schema/ABI, model hash, tokenizer identity, runtime commit, backend/device parameters, rank identity, and integrity checks match. Corruption or mismatch causes a miss/recomputation, never acceptance.
- **[RECOMMENDATION]** Distributed patches must declare rank ownership and failure behavior. A fabric/rank failure aborts the distributed request; it must not silently consume partially updated state. Single-node fallback starts a fresh compatible execution using that node's local cache only.
- **[RECOMMENDATION]** No commit on a published integration line may be intentionally unbuildable. Feature flags may keep incomplete facilities disabled, but the default testable configuration must remain bisectable.

## 15-known-gaps

- **[OPEN]** Section 13 has not yet approved the exact ROCmFPX donor commit-to-feature map.
- **[OPEN]** Section 14 has not yet approved which CachyLLama commits implement the required semantics without unrelated server behavior.
- **[OPEN]** The upstream commit from which ROCmFPX's current file tree was assembled is not proven by its current Git ancestry.
- **[OPEN]** No Halo fabric or HaloKV patch, owner, API, cache ABI, or validation result exists in the inspected sources.
- **[OPEN]** No two-machine build, performance, recovery, or failure-injection result is available. There are no **[MEASURED]** claims in this section.
