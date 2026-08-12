# Private release upload verification receipt — 2026-08-12

This immutable receipt records the complete byte-level audit of the draft
private preservation release before its tag was retargeted and before the
release was published. A later publication receipt must record the final tag
commit, workflow, and publication time. Do not rewrite this snapshot to make it
describe that later state.

## Release identity at audit

- repository: `JCFrags/HaloFPX`;
- repository visibility: private;
- release ID: `369369453`;
- tag name: `evidence-2026-08-12`;
- expected stable URL after publication:
  `https://github.com/JCFrags/HaloFPX/releases/tag/evidence-2026-08-12`;
- checked UTC: `2026-08-12T16:42:58.5195912+00:00`;
- state: draft;
- target at this audit: `c6c44d61c65e69e4b6cc23c0d10554116ba5fce3`.

[OPEN] That target was deliberately not treated as final. Publication remained
blocked until all continuation fixes were committed, the tag and draft release
were retargeted to that commit, and the complete workflow passed there.

## Remote asset reconciliation

The GitHub release API result was compared with the local release directory and
the tracked [`release-manifest.json`](release-manifest.json).

- remote assets: `41/41`;
- remote total: `23317868085` bytes;
- payload assets described by the manifest: `39/39`;
- state `uploaded`: `41/41`;
- GitHub-reported `sha256:` digests present: `41/41`;
- missing, extra, or duplicate names: `0`;
- local/remote size mismatches: `0`;
- local/remote SHA-256 mismatches: `0`;
- manifest payload disagreements: `0`.

Control-file identities:

| File | Bytes | SHA-256 |
|---|---:|---|
| `release-manifest.json` | `8251` | `317390d10c9194bb44cf769a596d0f5257772f6328d7118d0d1167c7461f0950` |
| `SHA256SUMS.txt` | `3876` | `cbeb29fb2e6cf6b45043bd17db2e925c1e0b4dcd5cff0c3d6f3250745708d827` |

The checksum file covers the 39 payloads and `release-manifest.json`.
Its own digest is anchored by this Git-tracked receipt because a checksum file
cannot include its own digest without circularity.

## Independent local restore verification

The hardened tracked verifier was run against the complete local release set:

```powershell
pwsh -NoProfile -File scripts/verify-publication-assets.ps1 `
  -AssetDirectory <complete-release-asset-directory> `
  -ManifestPath docs\publication\release-manifest.json
```

Result:

```text
Verified 39 assets, 2 split payloads, exact directory membership, and both trusted control files.
```

The verifier rehashed every payload, required the exact 41-file directory set,
bound both downloaded control files to the trusted tracked digests, rejected
unsafe names and reparse points, and reconstructed both logical split streams:

| Logical payload | Parts | Reconstructed bytes | SHA-256 |
|---|---:|---:|---|
| `l24-source-v2.tar` | `10` | `17101714432` | `5920dbdb2f1d29eac0be84c82611a9869318fae2ec5b3fe1392fd2ef9abef3cf` |
| `halofpx-project-p63-formal-evidence.tar.gz` | `2` | `2516292772` | `412dc86ea616b91e77b8618ffae3e4cadf9597c30a32fb91b5a2d3df41a98892` |

The source-role audit also verified all eight Git bundles, direct-source sizes
for 15 preserved implementation artifacts, and exact aggregate membership for
the six generated archives. The detailed mapping and restore boundary for every
payload is [`asset-provenance.json`](asset-provenance.json).

## Workflow observation before final closeout

GitHub Actions run
[`31616649927`](https://github.com/JCFrags/HaloFPX/actions/runs/31616649927)
passed at commit `c01b1f85fe2bf78eae8dedcff6d996c4c2ae5c83`:

- Wiki structure and validator tests: success;
- feature-off static build: success.

This run proved the corrected full-history workflow and feature-off build shape.
It did not satisfy the final publication gate because continuation fixes made
after `c01b1f8` still required their own run.

## Non-claims and exclusions

This audit does not authorize public redistribution, complete a license audit
of opaque archives, establish current production health, prove target-machine
runtime or model behavior, promote graph/RPC/scheduler/cache-product behavior,
or establish a performance result. The primary model remains absent at the
identity recorded in [`manifest.json`](manifest.json), and the Agent Harness
remains an external reference subject to its documented availability boundary.
