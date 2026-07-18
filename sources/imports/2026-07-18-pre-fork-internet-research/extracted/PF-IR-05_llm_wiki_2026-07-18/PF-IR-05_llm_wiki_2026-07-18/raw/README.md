# Raw and normalized primary-source captures

This tree preserves compact text/JSON captures of publisher repositories, converter repositories, and pinned runtime source. Captures are evidence extracts, not redistributed model weights and not full website mirrors.

Each capture records an immutable 40-hex revision and access date. `scripts/refresh.py` performs fail-closed Git fetches of those exact revisions with LFS smudge disabled. Large objects are materialized only with the explicit `--materialize-large-files` flag.

Capture types:

- `exact_small_file`: exact complete text file copied from a pinned source.
- `lfs_pointer_extract`: exact pointer metadata containing stored bytes and SHA-256.
- `partial_lfs_pointer_extract`: only a subset of pointers could be recovered; missing values remain `UNAVAILABLE`.
- `rendered_text_extract` / `normalized_field_extract`: normalized primary-page facts, with the immutable source URL retained in `manifests/source_registry.json`.

Rounded web UI sizes are never substituted for exact pointer bytes. Source presence is never substituted for local execution evidence.
