---
type: source-lock
status: candidate
created: 2026-07-17
gate: G0A
decision: OPEN-PIN-01 remains open
---

# Pre-fork source lock

Purpose: preserve the complete local Git evidence needed for the accepted L00A source/provenance-freeze lane without changing a reference clone, remote, target node, or donor source.

## Verdict

**[VERIFIED]** All four reference clones were complete and non-shallow; every named commit, tree, parent, and declared gitlink was inventoried; strict Git object checks passed; and status plus refs were identical before and after capture.

**[VERIFIED]** Four full all-ref offline bundles were created and independently accepted by git bundle verify. Their combined size is 845,587,914 bytes.

**[OPEN]** This is a complete G0A candidate evidence package, not gate closure. OPEN-PIN-01 still requires matched qualification before selecting ROCmFPX 61f2f2d7bc4955e9bca821095ef69125837133b5 or retaining a5605a72768c6562241b248e268e33dc92787394.

## Package map

- source-lock-manifest.json — canonical machine-readable identities, source roles, tree/parent/gitlink data, bundle identities, environment, and explicit OPEN-PIN-01 state.
- SHA256SUMS.txt — SHA-256 inventory for every package file except the checksum file itself.
- bundles/ — all refs present in each complete local clone at capture time.
- repository-records/ — status, refs, remotes, object availability, strict fsck, bundle verification, and license/build-input inventories.
- patch-ids/ — stable aggregate and per-commit patch identities plus commit/path summaries for the five relevant source deltas.
- commands.md — exact local commands, versions, and non-actions.
- source-lock-receipt.md — preservation and verification receipt.
- source-lock-review.md — closeout review and remaining gates.
- generate_source_lock.py — agent-authored deterministic collector; it invokes Git only against the evidence clones and writes only inside this package.

The four bundle files are intentionally workspace-local and ignored by Git because their combined size is 845,587,914 bytes. `SHA256SUMS.txt`, the manifest, verification output, and receipts retain their exact identities. Do not delete or relocate the workspace-local bundles without creating a superseding preservation receipt.

## Use boundary

The bundles are recovery/reference artifacts. Restoring one must target a new disposable directory. Do not restore into, fetch into, checkout, or otherwise alter the canonical reference clones. This package does not authorize a working fork, a remote, donor import, compilation, imported-tool execution, or any nimo-1/nimo-2 change.
