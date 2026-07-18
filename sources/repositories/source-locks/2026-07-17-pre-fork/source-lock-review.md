---
type: evidence-review
status: accepted-as-candidate
created: 2026-07-17
scope: G0A local source identity
---

# G0A candidate source-lock review

## Decision

**Accept as a complete candidate source-lock package for L00A evidence. Do not close G0A and do not begin implementation.**

## Review findings

- **[VERIFIED] Identity completeness:** the manifest records full commit and tree IDs, every parent, recursive tree gitlinks, local HEAD/branch, refs, remotes, shallow state, and before/after dirty state.
- **[VERIFIED] Object availability:** every locked commit/tree/parent resolved locally; strict full fsck passed in all four clones.
- **[VERIFIED] Offline preservation:** one all-ref bundle per clone passed git bundle verify. The bundle hashes in the receipt match the manifest.
- **[VERIFIED] Build/license traceability:** 1,839 deterministic inventory rows bind recognized license/notices and build/dependency/CI/container/orchestration inputs to Git objects and raw-blob SHA-256 values across the locked revisions.
- **[VERIFIED] Delta traceability:** five relevant ranges retain commit lists, changed paths, diff summaries, stable aggregate patch IDs, and stable per-commit IDs for non-merge commits.
- **[VERIFIED] Reversibility:** no source clone ref or worktree state changed; all writes are confined to this package and one routing addition.
- **[VERIFIED] Boundary compliance:** no network fetch, remote change, checkout, donor/imported-code execution, build, test, or Nimo access occurred.

## Limits and open gates

- **[OPEN] OPEN-PIN-01:** 61f2f2d7bc4955e9bca821095ef69125837133b5 is a qualification candidate only. It is not the selected implementation pin.
- **[OPEN] G0A:** source identities are now preserved, but the gate requires the selected candidate; therefore the gate cannot close from preservation evidence alone.
- **[OPEN] G0B/G0C/G1 and later gates:** repository governance, test-asset promotion, capability-level provenance/license approval, builds, implementation, persistence, and deployment remain unauthorized.
- Bundle verification proves Git structural prerequisites and advertised refs, not semantic equivalence, licensing permission, target compatibility, or a successful offline restoration rehearsal.

## Required next evidence

Run the separately authorized matched qualification lane for ROCmFPX research control versus implementation candidate. After a human-reviewed pin decision, update the source-lock decision record or create a superseding dated lock; do not silently rewrite this candidate package.

