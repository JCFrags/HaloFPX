# Procedures and Checks

## 1. Freeze the release-candidate snapshot

```bash
cutoff="$(date -u +%Y-%m-%dT%H:%M:%SZ)"
printf '%s
' "$cutoff" > provenance/accessed-at.txt

git ls-remote https://github.com/charlie12345/ROCmFPX.git refs/heads/main
git ls-remote https://github.com/fewtarius/llama-ai.git refs/heads/main
git ls-remote https://github.com/fewtarius/CachyLLama.git refs/heads/master
git ls-remote https://github.com/ggml-org/llama.cpp.git refs/heads/master
```

[RECOMMENDATION] Replace every mutable branch URL in release documentation with a 40-character commit URL.

## 2. Clone complete histories

```bash
mkdir -p audit-repos && cd audit-repos
git clone --no-single-branch https://github.com/charlie12345/ROCmFPX.git
git clone --recurse-submodules --no-single-branch https://github.com/fewtarius/llama-ai.git
git clone --no-single-branch https://github.com/fewtarius/CachyLLama.git
git clone --no-single-branch https://github.com/ggml-org/llama.cpp.git
```

Do not use a shallow clone for ancestry or file-history decisions.

## 3. Verify exact commits and ordered parents

```bash
# ROCmFPX
cd ROCmFPX
git cat-file -e a5605a72768c6562241b248e268e33dc92787394^{commit}

test "$(git rev-parse a5605a72768c6562241b248e268e33dc92787394^1)" = 25c71fc6e12d73bb3804127e032d29fb8976ae40
test "$(git rev-parse a5605a72768c6562241b248e268e33dc92787394^2)" = a8b5fa906ccd13c6a8ca06d55aa287854c376868

test "$(git rev-parse c2845bf86a5c1842d33bd9e990b2bcaf75779251^1)" = 5b3956605309dd3e6beed49c8f3a41423ba71d25
test "$(git rev-parse c2845bf86a5c1842d33bd9e990b2bcaf75779251^2)" = ccac6e55ec7c0fa8acdcb6e80b1a242e4f7d654e

test "$(git rev-parse 2335e6a482b1601d71dff9e860c8feab108c3af2^1)" = 221402af8574faf652b101b6afe225a3f329561f
test "$(git rev-parse 2335e6a482b1601d71dff9e860c8feab108c3af2^2)" = 5b3956605309dd3e6beed49c8f3a41423ba71d25

git merge-base --is-ancestor 2335e6a482b1601d71dff9e860c8feab108c3af2 a5605a72768c6562241b248e268e33dc92787394

git merge-base --is-ancestor 5fd2dc2c41c342a75c26f9756ca6b1814ed05fb4 a5605a72768c6562241b248e268e33dc92787394 \
  && echo 'EVIDENCE CHANGED: snapshot is now ancestor' \
  || echo 'expected at research cutoff: no ancestor relationship'

# CachyLLama
cd ../CachyLLama
test "$(git rev-parse 6be745998f568e379ea197fcf827baec73ff9940^1)" = c8ead677a7fe42fb0a67e6e866fb254cc338e9fd
test "$(git rev-parse 6be745998f568e379ea197fcf827baec73ff9940^2)" = 92366df30d4eaa4b85139b5fd694360237731b19
```

Archive `git cat-file -p` output for every merge. The remaining ROCmFPX question is not parent identity; it is the historical URL assigned to the local remote name `upstream`.

## 4. Verify submodules and hidden gitlinks

```bash
git show 1017f3dfdce3ca2b06aa9007b23295db3bb35722:.gitmodules
git ls-tree 1017f3dfdce3ca2b06aa9007b23295db3bb35722 CachyLLama
# Expected mode/type/SHA: 160000 commit 6be745998f568e379ea197fcf827baec73ff9940

# Run in every repository to find gitlinks even when .gitmodules is absent:
git ls-tree -r a5605a72768c6562241b248e268e33dc92787394 | awk '$1 == "160000" {print}'
```

[RECOMMENDATION] Fail CI when a gitlink lacks a manifest entry containing repository URL, exact commit, license, and notice path.

## 5. Build a complete file-provenance report

For each legally material file:

```bash
git log --all --follow --find-renames --find-copies   --format='%H%x09%aN%x09%aE%x09%aI%x09%s' -- path/to/file

git blame -w --line-porcelain a5605a72768c6562241b248e268e33dc92787394 -- path/to/file > blame.txt

git log --all -p -- path/to/file > history.patch
```

For suspected snapshot identity:

```bash
git rev-parse a5605a72768c6562241b248e268e33dc92787394:path/to/file
git --git-dir=../llama.cpp/.git rev-parse SOURCE_SHA:path/to/file
```

Equal blob IDs prove exact content identity at those commits; they do not by themselves prove graph ancestry.

## 6. License census

Run the supplied scanner:

```bash
../scripts/scan_licenses.sh /path/to/release-candidate audit-output
```

Then independently review:

```bash
find . -type f \( -iname 'LICENSE*' -o -iname 'COPYING*' -o -iname 'NOTICE*' \) -print
rg -n --hidden 'SPDX-License-Identifier|Licensed under|Public Domain|Unlicense|Copyright'   --glob '!*.lock' --glob '!build/**'
```

Optional recognized tools are invoked by the script when installed: REUSE, ScanCode Toolkit, Syft, and Licensee.

[RECOMMENDATION] Scanner output is triage evidence, not the legal conclusion. Manually inspect every “unknown,” generated file, binary, archive, and dual-license choice.

## 7. Vendor reproducibility check

```bash
python3 scripts/sync_vendor.py --help 2>/dev/null || true
sed -n '1,240p' scripts/sync_vendor.py
sha256sum vendor/**/* 2>/dev/null | sort > provenance/vendor-files.sha256
```

For each moving URL such as `releases/latest` or `master`:

1. resolve it once;
2. record final immutable commit/tag;
3. record downloaded SHA-256;
4. update the script to use that immutable reference;
5. preserve the upstream license file.

## 8. Synchronized ggml verification

```bash
marker="$(tr -d '[:space:]' < scripts/sync-ggml.last)"
printf 'ggml marker: %s
' "$marker"
git -C /path/to/ggml cat-file -e "$marker^{commit}"
```

[RECOMMENDATION] Diff the entire synchronized directory against the marker and record local modifications. A marker alone does not prove an unmodified copy.

## 9. ROCm/TheRock artifact audit

Before extracting:

```bash
sha256sum therock-dist-*.tar.gz > provenance/therock-archive.sha256
tar -tzf therock-dist-*.tar.gz | sort > provenance/therock-file-list.txt
```

After extracting/building:

```bash
find deps build -type f \( -iname 'LICENSE*' -o -iname 'COPYING*' -o -iname 'NOTICE*' \)   -print | sort > provenance/rocm-license-files.txt

find build/lib -type f -print0 | xargs -0 sha256sum | sort > provenance/bundled-libs.sha256

for bin in build/bin/*; do
  file "$bin"
  readelf -d "$bin" 2>/dev/null | rg 'NEEDED|RPATH|RUNPATH' || true
  ldd "$bin" 2>/dev/null || true
done > provenance/binary-linkage.txt

syft dir:build -o spdx-json=provenance/build.spdx.json
```

Create a component table with: file, SONAME, package/component, source repository, version/commit, license, required notice, source-offer requirement, checksum, and review owner.

[LEGAL-REVIEW] Do not release the self-contained archive until the table has no unknown component.

## 10. WebUI/npm audit

```bash
cd tools/ui
npm ci
npm ls --all --json > ../../provenance/npm-tree.json
npm sbom --sbom-format spdx > ../../provenance/npm.spdx.json 2>/dev/null || true
```

Use a license-reporting tool to export every direct and transitive package license. Inspect fonts, icons, example media, PDF workers, Mermaid, and any copied static assets separately.

For a prebuilt UI tarball, record:

- immutable URL and digest;
- source commit and build recipe;
- package-lock hash;
- dependency license report;
- embedded notice file;
- signature or trusted provenance attestation.

## 11. Notice generation and coverage

Start from [`templates/THIRD_PARTY_NOTICES.template.md`](templates/THIRD_PARTY_NOTICES.template.md), then run:

```bash
python3 scripts/check_notice_coverage.py   data/provenance_ledger.csv   /path/to/release/THIRD_PARTY_NOTICES.md
```

[RECOMMENDATION] Treat `AUTHORS`, Git history, and a root `LICENSE` as complementary evidence—not replacements for third-party notices.

## 12. Clean-room reimplementation check

For behavior sourced from GPL or CC-NC material:

1. freeze a functional specification containing no copied code/prose;
2. list every source the specification author reviewed;
3. separate specification and implementation personnel where practical;
4. prohibit source access for implementers during coding;
5. retain independent tests and design notes;
6. run similarity review against excluded source;
7. obtain patent and legal review;
8. sign a clean-room declaration.

## 13. Release gate

A release passes only when:

- branch tips and all dependency revisions are frozen;
- ordered merge parents are recorded and verified against `data/ancestry_edges.csv`;
- complete license scan has no unexplained files;
- every non-default license is in `LICENSES/` and `NOTICE`;
- GPL and CC-NC material is excluded or intentionally governed;
- ROCm/runtime and UI SBOMs are complete;
- model assets are separately approved;
- AI/contributor provenance review is closed;
- patent and trademark decisions are documented;
- counsel closes every blocker in the legal review register.

## Supplied automation

- [`scripts/verify_snapshot.sh`](scripts/verify_snapshot.sh)
- [`scripts/scan_licenses.sh`](scripts/scan_licenses.sh)
- [`scripts/check_notice_coverage.py`](scripts/check_notice_coverage.py)
- [`scripts/validate_wiki.py`](scripts/validate_wiki.py)
