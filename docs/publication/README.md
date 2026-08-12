# Publication Records

This directory holds machine-readable and release-side records for the combined
HaloFPX repository. It does not replace Project Lead authority.

- Start continuation with [`../../HANDOFF.md`](../../HANDOFF.md).
- Restore large payloads with [`../../ARTIFACTS.md`](../../ARTIFACTS.md).
- Read [`manifest.json`](manifest.json) for exact import commits, known absent
  resources, and publication-time preservation state.
- Read [`validation-2026-08-12.md`](validation-2026-08-12.md) for the scoped
  clean-publication validation and feature-off build receipt.
- Use [`release-manifest.json`](release-manifest.json) and
  [`SHA256SUMS.txt`](SHA256SUMS.txt) to verify the private evidence release.

## Current authority

The additive 2026-08-12 Project Lead
[decision](../../project/project-management/lead/DECISIONS.md#2026-08-12--accept-the-bounded-l111-loader-foundation)
accepts implementation commit `620ef60aa446990335ef46c7d76738f797e62f8f` as
`PASS / RETAIN` at the bounded L111 loader-foundation boundary only. It does not
promote graph, remote-procedure-call, runtime, model, production, cache,
product, or performance behavior.

The two verified local legacy bundles and their exact sizes and SHA-256 values
are recorded in [`../../ARTIFACTS.md`](../../ARTIFACTS.md). Their upload state
remains separate from their byte-level verification state.

## Update rule

When a private release is created or amended, add an immutable release asset
manifest that records:

- exact repository and release tag;
- asset name, purpose, size, SHA-256, and sensitivity class;
- ordered part names for every split payload;
- reconstructed size and SHA-256;
- source-relative path and retention rule; and
- restore verification result.

Do not edit a historical release receipt to describe a replacement asset. Add a
new receipt and state which earlier package it supersedes.
