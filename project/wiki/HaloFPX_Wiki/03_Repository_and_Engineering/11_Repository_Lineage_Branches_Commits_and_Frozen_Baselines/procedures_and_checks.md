---
section_id: "11"
title: "Repository Baseline Procedures and Checks"
status: "needs-machine-validation"
last_verified: "2026-07-16"
applies_to:
  repositories:
    - "fewtarius/llama-ai@1017f3dfdce3ca2b06aa9007b23295db3bb35722"
    - "fewtarius/CachyLLama@6be745998f568e379ea197fcf827baec73ff9940"
    - "charlie12345/ROCmFPX@a5605a72768c6562241b248e268e33dc92787394"
    - "ggml-org/llama.cpp@788e07dc91d266ad3162a1ce9037665656269689"
  software_versions:
    - "Git with bundle and submodule support"
  hardware_revisions: []
related_sections:
  - "15"
  - "16"
---

# Procedures and checks

All commands are non-destructive unless a destination already exists. No root access is required. Run in PowerShell 7+ from a dedicated source/archive directory; choose explicit paths rather than `$HOME` or a workspace root.

## 1. Observe remote state without cloning

```powershell
$repositories = @(
  'https://github.com/fewtarius/llama-ai.git',
  'https://github.com/fewtarius/CachyLLama.git',
  'https://github.com/charlie12345/ROCmFPX.git',
  'https://github.com/ggml-org/llama.cpp.git'
)

foreach ($repository in $repositories) {
  git ls-remote --symref $repository HEAD
}
```

**Pass condition:** each result contains one `ref: refs/heads/... HEAD` line and one full object ID. Record the command time, URL, branch, and SHA. Do not replace a previously frozen value when a remote advances (SRC-11-016).

## 2. Create complete mirrors

```powershell
$sourceRoot = 'D:\HaloFPX\source-mirrors'
New-Item -ItemType Directory -Force -Path $sourceRoot | Out-Null

git clone --mirror https://github.com/charlie12345/ROCmFPX.git `
  (Join-Path $sourceRoot 'ROCmFPX.git')
git clone --mirror https://github.com/fewtarius/CachyLLama.git `
  (Join-Path $sourceRoot 'CachyLLama.git')
git clone --mirror https://github.com/fewtarius/llama-ai.git `
  (Join-Path $sourceRoot 'llama-ai.git')
git clone --mirror https://github.com/ggml-org/llama.cpp.git `
  (Join-Path $sourceRoot 'llama.cpp.git')
```

If mirrors already exist, inspect their remotes before fetching. A mirror fetch can prune refs; do not use `--prune` during evidence preservation.

```powershell
git -C (Join-Path $sourceRoot 'ROCmFPX.git') remote -v
git -C (Join-Path $sourceRoot 'ROCmFPX.git') fetch --tags origin
git -C (Join-Path $sourceRoot 'ROCmFPX.git') fsck --full
```

**Pass condition:** the expected pinned commits exist and `git fsck --full` reports no corruption (SRC-11-015).

## 3. Verify commit and submodule identity

```powershell
$llamaAiMirror = Join-Path $sourceRoot 'llama-ai.git'
$llamaAiCommit = '1017f3dfdce3ca2b06aa9007b23295db3bb35722'

git -C $llamaAiMirror cat-file -e "$llamaAiCommit^{commit}"
git -C $llamaAiMirror ls-tree -r $llamaAiCommit |
  Select-String '^160000'
```

Expected gitlink:

```text
160000 commit 6be745998f568e379ea197fcf827baec73ff9940    CachyLLama
```

For a checked-out `llama-ai` worktree:

```powershell
git submodule sync --recursive
git submodule update --init --recursive
git submodule status --recursive
```

**Pass condition:** every submodule status begins with a space. A leading `-`, `+`, or `U` means uninitialized, mismatched, or conflicted state. Never use `git submodule update --remote` for a frozen build because it follows the branch hint instead of the recorded gitlink (SRC-11-014).

## 4. Record graph relationships

For graph-related repositories:

```powershell
$cacheMirror = Join-Path $sourceRoot 'CachyLLama.git'
git -C $cacheMirror remote add upstream https://github.com/ggml-org/llama.cpp.git
git -C $cacheMirror fetch upstream master

$cacheCommit = '6be745998f568e379ea197fcf827baec73ff9940'
$upstreamCommit = '788e07dc91d266ad3162a1ce9037665656269689'
$mergeBase = git -C $cacheMirror merge-base $cacheCommit $upstreamCommit
git -C $cacheMirror rev-list --left-right --count `
  "$mergeBase...$cacheCommit"
git -C $cacheMirror rev-list --left-right --count `
  "$mergeBase...$upstreamCommit"
```

For ROCmFPX, first test ancestry:

```powershell
$rocmMirror = Join-Path $sourceRoot 'ROCmFPX.git'
git -C $rocmMirror remote add upstream https://github.com/ggml-org/llama.cpp.git
git -C $rocmMirror fetch upstream master
git -C $rocmMirror merge-base `
  a5605a72768c6562241b248e268e33dc92787394 `
  788e07dc91d266ad3162a1ce9037665656269689
```

**Expected at this snapshot:** no merge base. Record `unrelated-history`; do not turn empty output into zero divergence.

## 5. Write a baseline manifest

**[RECOMMENDATION]** Commit one manifest using this minimum schema:

```yaml
schema: halofpx-source-baseline/v1
baseline_id: halofpx-baseline-YYYY-MM-DD.N
created_utc: YYYY-MM-DDTHH:MM:SSZ
repositories:
  - role: source-base
    url: https://github.com/charlie12345/ROCmFPX.git
    observed_default_branch: main
    commit: a5605a72768c6562241b248e268e33dc92787394
    tree: REPLACE_WITH_GIT_TREE_SHA
    tag_observed: null
    submodules: []
    bundle_sha256: REPLACE_WITH_SHA256
patch_stack:
  ledger_commit: REPLACE_WITH_FULL_SHA
dependencies:
  lock_digest_sha256: REPLACE_WITH_SHA256
build:
  recipe_commit: REPLACE_WITH_FULL_SHA
validation:
  receipt_path: experiments/REPLACE/receipt.json
```

Also record Git version/object format, remote fetch URLs, license-file hashes, dirty-state result, and whether commit/tag signatures were verified. A signature is useful provenance, but the full object ID remains required.

## 6. Freeze offline source bundles

```powershell
$bundleRoot = 'D:\HaloFPX\source-bundles\halofpx-baseline-YYYY-MM-DD.N'
New-Item -ItemType Directory -Force -Path $bundleRoot | Out-Null

$mirror = Join-Path $sourceRoot 'ROCmFPX.git'
$bundle = Join-Path $bundleRoot 'ROCmFPX.bundle'
git -C $mirror bundle create $bundle --all
git bundle verify $bundle
Get-FileHash -Algorithm SHA256 -LiteralPath $bundle
```

Repeat for all four repositories. Store the SHA-256 values in the manifest and copy bundles to at least one independent storage location (SRC-11-015).

**Constraint:** Git bundles do not include Git LFS objects, untracked files, downloaded dependencies, build outputs, or model weights. Inventory those separately with licenses and hashes under section 16.

## 7. Prove offline checkout

Use a new empty directory and disconnect or block network access for the proof:

```powershell
$restoreRoot = 'D:\HaloFPX\baseline-restore-test'
New-Item -ItemType Directory -Path $restoreRoot
git clone (Join-Path $bundleRoot 'ROCmFPX.bundle') `
  (Join-Path $restoreRoot 'ROCmFPX')
git -C (Join-Path $restoreRoot 'ROCmFPX') checkout --detach `
  a5605a72768c6562241b248e268e33dc92787394
git -C (Join-Path $restoreRoot 'ROCmFPX') status --porcelain=v2
git -C (Join-Path $restoreRoot 'ROCmFPX') fsck --full
```

**Pass condition:** detached HEAD is the manifest SHA, porcelain output is empty, and fsck passes. Restore and validate each submodule from its own bundle; do not allow a network fallback.

## 8. Build-validation handoff

Run section 16’s locked build and smoke tests from the detached restore on each node. Capture:

- node identity, hardware revision, BIOS/kernel/driver/toolchain;
- manifest and commit IDs;
- exact CMake/build/runtime flags;
- binary SHA-256 and linked-library inventory;
- pass/fail logs and timestamps.

No throughput or latency claim should be added to this section. Performance belongs in matched-configuration experiments with raw data.

## Freeze checklist

- [ ] Full 40-character commits resolve in local mirrors.
- [ ] Default branch observation is recorded but not used as the pin.
- [ ] Recursive gitlinks exactly match the manifest.
- [ ] Dirty, staged, untracked, and ignored build-input state is inventoried.
- [ ] Tag ref and resolved commit are stored separately.
- [ ] ROCmFPX unrelated-history condition is explicit.
- [ ] Source bundles verify and have SHA-256 receipts.
- [ ] Offline restore succeeds without implicit downloads.
- [ ] Licenses and third-party notices are preserved.
- [ ] Dependency/toolchain lock and validation receipt are linked.
- [ ] Baseline tag is annotated, immutable, and points to the manifest commit.
