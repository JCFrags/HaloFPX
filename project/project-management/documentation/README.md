# HaloFPX Documentation Control

This directory contains documentation inventory, navigation, validation, and organization records.

## Artifacts

- [`document-inventory.json`](document-inventory.json) records the pre-edit document inventory.
- [`navigation-map.md`](navigation-map.md) defines the required entry routes.
- [`validate_documentation.py`](validate_documentation.py) checks links, orphans, manifests, preservation, and readability.
- [`github_wiki_mirror.py`](github_wiki_mirror.py) generates and validates the
  non-authoritative GitHub Wiki convenience mirror from an exact Git commit.
- [`github-wiki-page-map.json`](github-wiki-page-map.json) freezes one unique
  mirror destination for every canonical Wiki source page and support asset.
- [`github-wiki-mirror-requirements.txt`](github-wiki-mirror-requirements.txt)
  pins the audited CommonMark parser stack by exact wheel hash.
- [`test_github_wiki_mirror.py`](test_github_wiki_mirror.py) exercises mirror
  coverage, deterministic rendering, link/anchor closure, tamper refusal, and
  offline validation.
- [`independent-review-2026-07-29.md`](independent-review-2026-07-29.md) records the read-only review and correction cycle.
- [`organization-receipt-2026-07-29.md`](organization-receipt-2026-07-29.md) records the completed organization pass.

The inventory baseline uses repository commit
`d30814ed08fe395f1bb1d292281ce82edb6bdab4`.
The inventory records unknown items without moving them.

## Validation modes

Run the validator from either the monorepo root or `project/`:

```powershell
python -X utf8 project/project-management/documentation/validate_documentation.py
python -X utf8 project-management/documentation/validate_documentation.py
```

The GitHub Wiki mirror is never canonical and its tool performs no GitHub or
network writes. From the monorepo root, verify the entire current source tree:

```powershell
python -m pip install --disable-pip-version-check --require-hashes -r project/project-management/documentation/github-wiki-mirror-requirements.txt
python project/project-management/documentation/github_wiki_mirror.py --repo . verify --source-ref HEAD
```

Use the publication runbook in
[`../../../docs/publication/github-wiki-mirror.md`](../../../docs/publication/github-wiki-mirror.md)
before any manual GitHub Wiki bootstrap, publication, drift recovery, or
visibility change.

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
