# HaloFPX Documentation Control

This directory contains documentation inventory, navigation, validation, and organization records.

## Artifacts

- [`document-inventory.json`](document-inventory.json) records the pre-edit document inventory.
- [`navigation-map.md`](navigation-map.md) defines the required entry routes.
- [`validate_documentation.py`](validate_documentation.py) checks links, orphans, manifests, preservation, and readability.
- [`independent-review-2026-07-29.md`](independent-review-2026-07-29.md) records the read-only review and correction cycle.
- [`organization-receipt-2026-07-29.md`](organization-receipt-2026-07-29.md) records the completed organization pass.

The inventory baseline uses repository commit
`d30814ed08fe395f1bb1d292281ce82edb6bdab4`.
The inventory records unknown items without moving them.

## Validation modes

Run the validator from either the monorepo root or `project/`:

```powershell
python project/project-management/documentation/validate_documentation.py
python project-management/documentation/validate_documentation.py
```

In the former standalone documentation worktree, the validator recomputes the
complete protected-area metadata and byte digest, including the untracked
payloads present when the inventory was recorded. In the combined monorepo,
those workstation-only payloads are not ordinary Git objects. Publication mode
therefore verifies all of the following instead of pretending to rehash absent
bytes:

- the exact implementation, engineering-wiki, and two-parent integration
  commits are ancestors of `HEAD`;
- the `project/` tree at the integration commit exactly equals the imported
  engineering-wiki commit tree;
- the original inventory is byte-identical apart from newline normalization;
- the Git-tracked protected subtrees have not changed since integration and
  have no tracked or untracked worktree changes; and
- [`../../../docs/publication/manifest.json`](../../../docs/publication/manifest.json)
  binds the original protected snapshot's file count and digests.

The report distinguishes the number of imported Git files it can directly
check from the larger legacy snapshot count. Supplemental release-asset
verification remains the responsibility of the publication manifests and does
not convert an absent payload into a successful content check.
