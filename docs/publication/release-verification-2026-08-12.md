# Private Release Publication Verification — 2026-08-12

## Scope

This additive receipt records the final publication state of HaloFPX's private
preservation release. It does not change the byte manifests, replace the
historical draft-upload receipt, authorize public redistribution, or promote
any implementation claim beyond the accepted L111 loader-foundation boundary.

## Immutable release identity

- repository: `JCFrags/HaloFPX`;
- visibility at verification: private;
- release: `HaloFPX private preservation set — 2026-08-12`;
- tag: `evidence-2026-08-12`;
- URL:
  `https://github.com/JCFrags/HaloFPX/releases/tag/evidence-2026-08-12`;
- GitHub release database ID: `369369453`;
- GitHub release node ID: `RE_kwDOT2ckv84WBCFt`;
- release boundary commit:
  `7c801894062c2e09122c18447da66d50da60c050`;
- annotated tag object:
  `ddbdc98d5a98ea19fd96daf11d848360721aae74`;
- annotated tag peeled commit:
  `7c801894062c2e09122c18447da66d50da60c050`;
- created: `2026-08-12T17:22:48Z`;
- published: `2026-08-12T17:25:51Z`;
- final state: `draft=false`, `immutable=true`.

[VERIFIED] Immediately before publication, local `HEAD`, `origin/main`, the
release target, and the peeled annotated tag all equaled the release boundary
commit. GitHub immutable releases were enabled before the one-time draft-to-
published transition. After publication, the remote tag object and peeled
commit were read again and retained the identities above.

The tag is the immutable release boundary. This receipt and later development
commits are intentionally descendants of that boundary and must not cause the
published tag to move. A changed artifact set requires a new tag and release.

## Validation gate

[VERIFIED] GitHub Actions run
[`31621950447`](https://github.com/JCFrags/HaloFPX/actions/runs/31621950447)
completed successfully at the exact release boundary commit:

- `Wiki structure and validator tests`: success;
- `Feature-off static build`: success.

The wiki job checked the generated manifest, all wiki structure and schemas,
all focused validator tests, and documentation/protected provenance. The build
job configured the feature-off tree and compiled the core `llama` target.

## Artifact reconciliation

[VERIFIED] The published release has an exact, case-sensitive inventory of 41
uploaded files totaling `23317868085` bytes: 39 payload assets plus the two
trusted control files. Before publication, every local file was rehashed and
its name, size, state, and SHA-256 were reconciled with the committed controls
and GitHub's reported digest. The final post-publication inventory retained the
same count and total.

| Trusted control | Size (bytes) | SHA-256 |
|---|---:|---|
| `release-manifest.json` | `8251` | `317390d10c9194bb44cf769a596d0f5257772f6328d7118d0d1167c7461f0950` |
| `SHA256SUMS.txt` | `3876` | `cbeb29fb2e6cf6b45043bd17db2e925c1e0b4dcd5cff0c3d6f3250745708d827` |

The full per-asset authority remains
[`release-manifest.json`](release-manifest.json),
[`SHA256SUMS.txt`](SHA256SUMS.txt), and
[`asset-provenance.json`](asset-provenance.json). The retained split-payload
whole-file identities are:

- `l24-source-v2.tar`: `17101714432` bytes, SHA-256
  `5920dbdb2f1d29eac0be84c82611a9869318fae2ec5b3fe1392fd2ef9abef3cf`;
- `halofpx-project-p63-formal-evidence.tar.gz`: `2516292772` bytes,
  SHA-256
  `412dc86ea616b91e77b8618ffae3e4cadf9597c30a32fb91b5a2d3df41a98892`.

## GitHub release attestation

[VERIFIED] `gh release verify evidence-2026-08-12 --repo JCFrags/HaloFPX
--format json` succeeded after publication. GitHub's in-toto release
attestation binds the package to annotated tag object
`ddbdc98d5a98ea19fd96daf11d848360721aae74` and lists all 41 release assets
with their SHA-256 digests. The returned verification result included a trusted
timestamp of `2026-08-12T17:25:52Z` and the GitHub releases certificate
identity.

## Explicit exclusions and test limits

- [OPEN] The primary model is absent. Its retained identity is
  `159873097824` bytes and SHA-256
  `96506ada918e60ca9a9cfde8a5437790e4453401a6a3e236e3f55e7bac3aaea6`.
- [OPEN] The external Agent Harness authority was unavailable and was not
  imported. Continue through `project/references/agent-harness.md`.
- Generated build caches, credentials, and live service state are intentionally
  excluded.
- NOT RUN: GPU, two-node, primary-model, production, and performance gates —
  publication changed preservation metadata, not those behaviors.
- REQUIRED BEFORE CLAIM: run the focused hardware, model, distributed, or
  performance gate when a future milestone actually depends on that claim.

The repository and release must remain private until a separate privacy,
license, notice, and redistribution review explicitly authorizes a public
subset.
