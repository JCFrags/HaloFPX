---
section_id: "15"
title: "Integration procedures and checks"
status: "needs-machine-validation"
last_verified: "2026-07-16"
applies_to:
  repositories:
    - "HaloFPX integration repository (proposed)"
    - "ggml-org/llama.cpp@788e07dc91d266ad3162a1ce9037665656269689"
    - "charlie12345/ROCmFPX@a5605a72768c6562241b248e268e33dc92787394"
    - "fewtarius/CachyLLama@6be745998f568e379ea197fcf827baec73ff9940"
    - "fewtarius/llama-ai@1017f3dfdce3ca2b06aa9007b23295db3bb35722"
  software_versions: ["Git 2.54.x procedure semantics"]
  hardware_revisions: ["two matched AMD Strix Halo systems; exact revisions open"]
related_sections: ["11", "13", "14", "16"]
---

# Integration procedures and checks

All examples are non-root Git operations. Run them in a clean disposable clone with Git 2.54.x or record the actual `git --version`. Replace example origin URL and branch names only after the implementation repository exists. Commands fetch and create local refs; none deletes a branch or force-pushes.

## 15-verify-pinned-sources

```bash
git ls-remote --symref https://github.com/ggml-org/llama.cpp.git HEAD
git ls-remote --symref https://github.com/charlie12345/ROCmFPX.git HEAD
git ls-remote --symref https://github.com/fewtarius/CachyLLama.git HEAD
git ls-remote --symref https://github.com/fewtarius/llama-ai.git HEAD
```

Compare the advertised default ref and SHA to the source ledger. The authoritative object verification is to fetch a named branch/tag, run `git cat-file -e <sha>^{commit}`, and verify the expected ref still contains the pinned object. A moved branch does not invalidate an already fetched content-addressed commit.

```bash
git fetch --no-tags llama master
git cat-file -e 788e07dc91d266ad3162a1ce9037665656269689^{commit}
git merge-base --is-ancestor \
  788e07dc91d266ad3162a1ce9037665656269689 llama/master || true
```

Record full command output, access time, remote URL, and commit object in the source evidence layer before promoting a new baseline.

## 15-bootstrap-remotes-and-immutable-input-tags

```bash
git remote add llama https://github.com/ggml-org/llama.cpp.git
git remote add rocmfpx https://github.com/charlie12345/ROCmFPX.git
git remote add cachy https://github.com/fewtarius/CachyLLama.git
git fetch --no-tags llama master
git fetch --no-tags rocmfpx main
git fetch --no-tags cachy master

git tag -a baseline/llama-788e07dc \
  788e07dc91d266ad3162a1ce9037665656269689 \
  -m "llama.cpp research anchor 2026-07-16"
git tag -a donor/rocmfpx-a5605a72 \
  a5605a72768c6562241b248e268e33dc92787394 \
  -m "ROCmFPX donor snapshot 2026-07-16"
git tag -a donor/cachy-6be74599 \
  6be745998f568e379ea197fcf827baec73ff9940 \
  -m "CachyLLama donor snapshot 2026-07-16"
```

Do not push tags until their manifest and source records are reviewed. If signing is configured, use signed annotated tags for accepted baselines/releases.

## 15-ancestry-and-divergence-gate

```bash
git merge-base \
  a5605a72768c6562241b248e268e33dc92787394 \
  788e07dc91d266ad3162a1ce9037665656269689

git merge-base \
  6be745998f568e379ea197fcf827baec73ff9940 \
  788e07dc91d266ad3162a1ce9037665656269689

git rev-list --left-right --count \
  6be745998f568e379ea197fcf827baec73ff9940...\
788e07dc91d266ad3162a1ce9037665656269689
```

Expected for the research snapshot: ROCmFPX returns no merge base; CachyLLama returns `92366df30d4eaa4b85139b5fd694360237731b19`; Cachy/llama counts are `53 125`. Any different output means the objects or assumptions differ and must be investigated before integration.

## 15-port-commit-requirements

Every lane commit message should include trailers equivalent to:

```text
HaloFPX-Lane: 10-rocmfpx
Source-Repo: https://github.com/charlie12345/ROCmFPX
Source-Commit: <40-character-sha>
Source-Paths: <comma-separated paths>
Source-License: MIT
Port-Mode: unchanged|adapted|reimplemented
Depends-On: <patch-id or none>
Upstream-Status: downstream|submitted:<url>|accepted:<sha>|not-applicable
Validation: <test IDs; no invented result>
```

Use `git patch-id --stable` to inventory equivalent diffs where possible. Patch IDs do not establish semantic equivalence for adapted code, merge commits, generated files, or context-dependent behavior.

## 15-rebase-and-range-review

```bash
git switch topic/10-rocmfpx
old_tip=$(git rev-parse HEAD)
git branch archive/topic-10-before-sync-$old_tip "$old_tip"
git rebase --rebase-merges baseline/llama-<new-shortsha>
new_tip=$(git rev-parse HEAD)
git range-diff \
  baseline/llama-<old-shortsha>.."$old_tip" \
  baseline/llama-<new-shortsha>.."$new_tip"
```

Review unmatched, reordered, dropped, and materially changed patches. Save the range-diff with the candidate evidence. `git rebase --abort` is the safe exit while a rebase is in progress. Do not use this procedure on a published candidate or release.

Optional local conflict assistance:

```bash
git config rerere.enabled true
git rerere status
git rerere diff
```

An automatically staged reuse still requires owner review and the complete lane gate.

## 15-cherry-pick-checklist

Before `git cherry-pick -x <sha>`:

1. Read the commit and all parents; reject merge commits unless the explicit mainline and rationale are reviewed.
2. Confirm repository/file license and preserve authorship.
3. List dependency commits and affected lanes.
4. Inspect `git show --stat --summary <sha>` and the full diff.
5. Apply in a disposable topic branch with `-x`.
6. Add HaloFPX provenance trailers in a follow-up amendment only if repository policy permits it.
7. Run source tests plus the destination lane gate.
8. Compare behavior/configuration defaults; a clean textual application is not semantic proof.

## 15-compose-a-candidate

```bash
git switch --detach baseline/llama-<shortsha>
git switch -c candidate/<date>-<shortsha>
git merge --no-ff topic/10-rocmfpx -m "stack: integrate lane 10 ROCmFPX"
git merge --no-ff topic/20-cachy-semantics -m "stack: integrate lane 20 cache semantics"
git merge --no-ff topic/30-halo-fabric -m "stack: integrate lane 30 Halo fabric"
git merge --no-ff topic/40-halokv -m "stack: integrate lane 40 HaloKV"
git merge --no-ff topic/50-product -m "stack: integrate lane 50 product"
```

After each merge, run that lane's fast gate. Run the full clean build/test/machine matrix on the final candidate. Freeze the remote candidate ref once review starts; a fix creates a topic commit and a new candidate rather than rewriting history.

## 15-conflict-record

For every nontrivial conflict, record:

- old/new upstream anchors and lane tips;
- affected paths and conflict class;
- upstream intent and downstream intent;
- chosen resolution and rejected alternatives;
- owners/reviewers;
- tests and raw evidence;
- whether `rerere` suggested the resolution;
- reusable upstream or interface improvement.

If intent cannot be established from source/tests, abort and open a question; do not guess.

## 15-bisect-rehearsal

Prerequisites: a deterministic non-destructive classifier returning `0` for good, `1-127` except `125` for bad, and `125` for untestable commits.

```bash
git bisect start <known-bad> <known-good>
git bisect run ./scripts/halofpx-bisect-gate.sh
git bisect log
git bisect reset
```

Preserve the script, bisect log, build configuration, model hash if used, machine identity, and raw outputs. Excessive `skip` results or an unbuildable commit means the patch series is not acceptably bisectable.

## 15-machine-validation-matrix

These are required future experiments; none has been run by this research task.

| ID | Required check | Environment/evidence | Pass condition |
|---|---|---|---|
| `EXP-15-01` | Clean upstream then per-lane builds | Both machines; compiler/ROCm/kernel/firmware and full SHAs | Each boundary builds and its declared tests pass |
| `EXP-15-02` | ROCmFPX donor-to-port completeness | Pinned donor, anchor, path manifest, patch IDs/diffs | Every donor item included or explicitly excluded; reference/backend gates pass |
| `EXP-15-03` | Cache compatibility and corruption | Model/tokenizer hashes, cache manifests, raw fault-injection logs | Mismatch/truncation/checksum/schema/rank errors always miss or recompute |
| `EXP-15-04` | Dual-link/rank failure | Two links; per-rank logs; link-down and process-kill cases | Distributed request fails coherently; no invalid cache accepted; documented single-node fallback succeeds fresh |
| `EXP-15-05` | Seeded bisect | Known injected fault in one lane | Automated bisect identifies the patch; all intermediate commits are classifiable |
| `EXP-15-06` | Upgrade and rollback | Previous and candidate release artifacts | Upgrade invalidates/migrates state as specified; rollback restores service without accepting incompatible cache |

## 15-closeout-review

Before release, review every modified artifact for correctness, freshness, clarity, provenance, ownership, reversible rollback, and reusable improvement. Apply a small evidence-backed correction within the candidate; otherwise record a proposal rather than silently altering trusted material.
