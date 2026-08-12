# HaloFPX Artifact Preservation and Restore Guide

The Git repository is the canonical home for source, decisions, wiki pages, and
small evidence. Private GitHub release assets are supplemental preservation for
large or awkward payloads. An asset is not authoritative merely because its
filename resembles a historical file.

## Preservation classes

| Class | Status | Recovery rule |
|---|---|---|
| Combined Git history | [VERIFIED] included | Clone `main`; verify both imported ancestors and the integration boundary. |
| Engineering wiki and tracked imports | [VERIFIED] included under `project/` | Use Git paths and commit identities, not a GitHub Wiki mirror. |
| Implementation evidence tracked at `620ef60...` | [VERIFIED] included | Use `docs/halofpx/` and its immutable receipts. |
| Pre-publication Git bundles | [VERIFIED] uploaded to private release `evidence-2026-08-12`; remote size and SHA-256 matched | Accept only the exact names, sizes, and SHA-256 values recorded below. |
| Large raw source/formal/evidence payloads | [VERIFIED] uploaded to private release `evidence-2026-08-12`; remote size and SHA-256 matched | Restore only from a complete asset set and verify every part plus the reconstructed payload. |
| Generated build trees and caches | [RECOMMENDATION] rebuild, not canonical | Do not treat local build output as a transferable source of truth. |
| Primary model | [OPEN] absent | Obtain exact bytes independently; verify size and SHA-256 before use. |
| Credentials and live service state | intentionally excluded | Re-provision through the current operator authority; never restore from project artifacts. |

The machine-readable companions are the publication
[`manifest.json`](docs/publication/manifest.json), exact byte
[`release-manifest.json`](docs/publication/release-manifest.json), tracked
[`SHA256SUMS.txt`](docs/publication/SHA256SUMS.txt), and per-asset
[`asset-provenance.json`](docs/publication/asset-provenance.json). Together they
are the packaging, source-role, sensitivity, retention, and restore authority.
They cannot supersede project decisions.

## Git history preservation

The combined repository preserves these exact commits:

- implementation: `620ef60aa446990335ef46c7d76738f797e62f8f`;
- engineering wiki: `b1c2d8aef707fb03920fc189ccd26395fa61879d`;
- two-parent integration boundary:
  `728c3b441fcb38a9eb55272ed673da9d2d18c173`.

Two complete pre-publication bundles were created and verified locally before
integration:

| Bundle | Size (bytes) | SHA-256 | Publication state |
|---|---:|---|---|
| `implementation-620ef60.bundle` | `41317318` | `b69bbcb25b5d21fa5551e9a66fdd4ba69a8914973e35f9695c640f31652e322b` | [VERIFIED] private-release asset |
| `wiki-b1c2d8a.bundle` | `39827352` | `68405b7510450898a5ec2b690431c204ed56ba76eaba5ef2d2bab42368ab4293` | [VERIFIED] private-release asset |

The normal clone is sufficient for continuing work; the bundles reconstruct
the former separate repositories. Before using a downloaded bundle, require
the exact size and SHA-256 above even when its filename matches.

After downloading verified bundles into an empty scratch directory, initialize
a disposable repository for `git bundle verify`:

```powershell
git init .\bundle-verifier
git -C .\bundle-verifier bundle verify ..\implementation-620ef60.bundle
git -C .\bundle-verifier bundle verify ..\wiki-b1c2d8a.bundle
git clone --branch codex/integration-base-61f2f2d .\implementation-620ef60.bundle .\HaloFPX-implementation-legacy
git clone --branch codex/pre-fork-preparation .\wiki-b1c2d8a.bundle .\HaloFPX-project-legacy
git -C .\HaloFPX-implementation-legacy cat-file -t 620ef60aa446990335ef46c7d76738f797e62f8f
git -C .\HaloFPX-project-legacy cat-file -t b1c2d8aef707fb03920fc189ccd26395fa61879d
```

The explicit branches are required because these two bundles do not advertise
a symbolic `HEAD` that `git clone` can select automatically.

### All-refs bundle recovery

The donor bundles named `--all-refs.bundle` intentionally retain refs outside
`refs/heads/*`, including captured `refs/remotes/origin/*` names. Plain
`git clone` does not reproduce that ref namespace. To reconstruct one exactly,
initialize a new scratch repository, move its checked-out `HEAD` out of the
incoming namespace, fetch the complete refspec, then set `HEAD` explicitly:

```powershell
git init .\donor-recovery
git -C .\donor-recovery symbolic-ref HEAD refs/heads/recovery-bootstrap
git -C .\donor-recovery fetch ..\charlie12345__rocmfpx--all-refs.bundle '+refs/*:refs/*'
git -C .\donor-recovery symbolic-ref HEAD refs/heads/main
git bundle list-heads .\charlie12345__rocmfpx--all-refs.bundle
git -C .\donor-recovery for-each-ref --format='%(objectname) %(refname)'
```

Compare every advertised non-`HEAD` ref and object ID with `for-each-ref`.
For the CachyLlama and retained llama.cpp bundles, set the final symbolic
`HEAD` to `refs/heads/master`; for llama-ai use `refs/heads/main`. Never
mirror-push these preserved ref sets.

Do not push bundle refs with `--mirror`. Historical local checkpoint refs in the
former documentation repository included large blobs that were not part of the
accepted branch. The combined branch intentionally preserves accepted branch
history rather than every workstation-only ref.

## Large payload groups

The private `evidence-2026-08-12` release preserves these groups. Exact archive
roots, member recipes, sensitivity classes, and restore destinations are in
[`asset-provenance.json`](docs/publication/asset-provenance.json).

| Group | Purpose | Packaging requirement |
|---|---|---|
| Former documentation-repository raw sources | Preserve untracked source captures and provenance | Archive by stable relative path; include per-file or archive SHA-256. |
| P63 formal artifacts and tool distributions | Preserve model-checking inputs/results and the exact tool packages available locally | Keep tool licenses and receipts beside the archive; do not imply current verification from a retained state file. |
| Former implementation-repository raw evidence | Preserve historical L83/L85/L97/L98 and other ignored/untracked evidence without polluting Git history | Preserve relative paths and immutable bytes; record whether the payload contains machine identifiers. |
| Restricted paired-machine manifests | Preserve private operational provenance | Private release only; restore to a restricted evidence area, never an active configuration directory. |
| Pre-publication Git bundles | Reconstruct the two original repositories | Upload whole; verify as Git bundles and by SHA-256. |

One known raw source capture exceeds ordinary single-asset limits:

- former relative path: `sources/halofpx/l24-source-v2.tar`;
- size: `17101714432` bytes;
- SHA-256:
  `5920dbdb2f1d29eac0be84c82611a9869318fae2ec5b3fe1392fd2ef9abef3cf`.

It must be split into deterministic, ordinal parts smaller than the host's
single-asset limit. The release manifest must record the exact ordered part
names, size, and SHA-256 for every part, plus the reconstructed file's identity
above. Do not compress or rewrite this tar merely to make its digest differ;
preservation requires restoration of the original bytes.

## Download and trust procedure

1. Use an account authorized for the private repository.
2. Download the release manifest and checksum file before bulk payloads.
3. Compare the manifest's repository, release tag, asset names, split-payload
   `logical_name` values, part counts, part order, sizes, and hashes with the
   Git-tracked publication record.
4. Download into a new scratch directory with enough free space for both the
   parts and reconstructed payload.
5. Hash each downloaded part before concatenation.
6. Concatenate only in the manifest's explicit order.
7. Hash the reconstructed payload and compare its size and SHA-256.
8. Inspect archive entry names before extraction. Reject absolute paths,
   traversal components, unexpected links, or collisions.
9. Extract to a new directory. Never overlay an active worktree or live service
   configuration.
10. Preserve the downloaded manifest and hashes as the restore receipt.

Example private-release download after selecting an exact tag:

```powershell
gh api repos/JCFrags/HaloFPX/releases/tags/evidence-2026-08-12 --jq '{id,tag_name,target_commitish,draft,prerelease,immutable,published_at}'
git ls-remote https://github.com/JCFrags/HaloFPX.git refs/tags/evidence-2026-08-12 'refs/tags/evidence-2026-08-12^{}'
# Compare target_commitish and the peeled tag commit with the publication receipt.
New-Item -ItemType Directory -Path .\halofpx-release
gh release download evidence-2026-08-12 --repo JCFrags/HaloFPX --dir .\halofpx-release --pattern release-manifest.json --pattern SHA256SUMS.txt
Get-FileHash .\halofpx-release\release-manifest.json -Algorithm SHA256
Get-FileHash .\halofpx-release\SHA256SUMS.txt -Algorithm SHA256
# Compare both results with docs/publication/release-upload-verification-2026-08-12.md,
# then download payloads without overwriting the verified control files.
gh release download evidence-2026-08-12 --repo JCFrags/HaloFPX --dir .\halofpx-release --skip-existing
pwsh -NoProfile -File scripts/verify-publication-assets.ps1 -AssetDirectory .\halofpx-release -ManifestPath docs/publication/release-manifest.json
```

Do not use an unpinned `latest` release for evidence restoration.

## Reconstructing split payloads on Windows

Use the part order from the release manifest; do not infer order from download
timestamps. This PowerShell pattern streams bytes and avoids loading a part into
memory:

```powershell
$partNames = @(
    # Copy the exact ordered names from the release manifest.
    "artifact.part-0001",
    "artifact.part-0002"
)
$outputPath = Join-Path $PWD "restored-artifact.tar"
$output = [System.IO.File]::Create($outputPath)
try {
    foreach ($partName in $partNames) {
        $input = [System.IO.File]::OpenRead((Join-Path $PWD $partName))
        try {
            $input.CopyTo($output)
        }
        finally {
            $input.Dispose()
        }
    }
}
finally {
    $output.Dispose()
}
Get-Item $outputPath | Select-Object FullName, Length
Get-FileHash -Algorithm SHA256 $outputPath
```

Replace the example names with the complete manifest order. For the known L24
tar, accept the result only when length is `17101714432` and SHA-256 is
`5920dbdb2f1d29eac0be84c82611a9869318fae2ec5b3fe1392fd2ef9abef3cf`.

The other split payload is
`halofpx-project-p63-formal-evidence.tar.gz`. Accept it only when length is
`2516292772` and SHA-256 is
`412dc86ea616b91e77b8618ffae3e4cadf9597c30a32fb91b5a2d3df41a98892`.

On GNU/Linux with Bash and GNU coreutils, enter the downloaded asset directory,
stream parts in the manifest order, and verify the result:

```bash
cd halofpx-release
printf '%s\0' l24-source-v2.tar.part{0001..0010} |
  xargs -0 cat -- > l24-source-v2.tar
test "$(stat -c %s l24-source-v2.tar)" = 17101714432
printf '%s  %s\n' \
  5920dbdb2f1d29eac0be84c82611a9869318fae2ec5b3fe1392fd2ef9abef3cf \
  l24-source-v2.tar | sha256sum --check --strict
```

The brace expression above is valid only for this exact recorded ten-part L24
payload. For any other split payload, copy the ordered names from the manifest
instead of inferring them.

## Model identity and absence

The primary model bytes were not available to the publication workspace and
therefore cannot be recovered from Git or a project release. The only retained
identity is:

```text
size_bytes=159873097824
sha256=96506ada918e60ca9a9cfde8a5437790e4453401a6a3e236e3f55e7bac3aaea6
```

The name of a model, quantization label, repository branch, or download URL is
not sufficient identity. A successor must verify both fields before using a
candidate as the primary-model input.

## Deliberate exclusions and boundaries

- Generated build directories, compiler caches, and dependency caches are
  workstation scratch. Rebuild them from exact source and toolchain receipts.
- Live credentials, service secrets, private keys, and authenticated browser or
  command-line sessions are never continuity artifacts.
- Live service state and a historical `systemd` observation are not portable.
  Re-provision only through current operator authority.
- `Agent_Harness` is an external reference authority, not a subtree imported by
  this publication. Follow
  [`project/references/agent-harness.md`](project/references/agent-harness.md)
  and resolve an authorized copy separately.
- Third-party imports remain governed by their own licenses. Private retention
  does not create permission for public redistribution.

If a required release group is absent or a digest is missing, record the gap as
`[OPEN]`. Do not recreate raw evidence from a summary and do not promote a
synthetic replacement to `[MEASURED]` or `[VERIFIED]`.
