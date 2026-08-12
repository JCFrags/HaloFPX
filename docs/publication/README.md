# Publication Records

This directory holds machine-readable and release-side records for the combined
HaloFPX repository. It does not replace Project Lead authority.

- Start continuation with [`../../HANDOFF.md`](../../HANDOFF.md).
- Restore large payloads with [`../../ARTIFACTS.md`](../../ARTIFACTS.md).
- Read [`manifest.json`](manifest.json) for exact import commits, known absent
  resources, and publication-time preservation state.
- Read [`validation-2026-08-12.md`](validation-2026-08-12.md) for the scoped
  clean-publication validation and feature-off build receipt.
- Read
  [`release-upload-verification-2026-08-12.md`](release-upload-verification-2026-08-12.md)
  for the immutable 41-file draft-release digest reconciliation.
- Use [`release-manifest.json`](release-manifest.json) and
  [`SHA256SUMS.txt`](SHA256SUMS.txt) to verify the private evidence release.
- Use [`asset-provenance.json`](asset-provenance.json) to map every payload to
  its source role, purpose, sensitivity, retention rule, and safe restore path.

## Current authority

The additive 2026-08-12 Project Lead
[decision](../../project/project-management/lead/DECISIONS.md#2026-08-12--accept-the-bounded-l111-loader-foundation)
accepts implementation commit `620ef60aa446990335ef46c7d76738f797e62f8f` as
`PASS / RETAIN` at the bounded L111 loader-foundation boundary only. It does not
promote graph, remote-procedure-call, runtime, model, production, cache,
product, or performance behavior.

The two legacy bundles are verified private release assets. Their remote names,
sizes, and GitHub-reported SHA-256 values matched the local bytes recorded in
[`../../ARTIFACTS.md`](../../ARTIFACTS.md).

## Update rule

When a private release is created or amended, maintain a collective immutable
record consisting of the byte manifest, checksum file, provenance map, and
release verification receipt. Together they must record:

- exact repository and release tag;
- asset name, purpose, size, SHA-256, and sensitivity class;
- ordered part names for every split payload;
- reconstructed size and SHA-256;
- source-relative path and retention rule; and
- restore verification result.

Do not edit a historical release receipt to describe a replacement asset. Add a
new receipt and state which earlier package it supersedes.
