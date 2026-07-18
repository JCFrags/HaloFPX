# Evidence index

The machine-readable source manifest is in `manifests/source_manifest.csv` and `source_manifest.json`. Each evidence file begins with metadata recording source class, literal claim label, repository/site, path/URL, ref, upstream object SHA where available, locator, access date, completeness, and license.

Key paths:

* `evidence/source/active/` — active 2.30.4 source and API excerpts.
* `evidence/source/stable/` — 2.27.7 release source and API excerpts.
* `evidence/docs/` — official documentation snapshots and lane map.
* `evidence/tests/` — upstream test-scope evidence.
* `evidence/issues/` — attributed issue and PR snapshots.
* `claims/claims.csv` / `.json` / `.jsonl` — literal claim registry.
* `licenses/` — complete captured RCCL license files and license notes.
* `manifests/SHA256SUMS` — local package integrity list.
