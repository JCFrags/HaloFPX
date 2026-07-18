# Reproducible refresh and validation helpers

All repository identities in the manifests are full 40-hex commits. `refresh.py` rejects moving refs and uses `GIT_LFS_SKIP_SMUDGE=1`. Large files are only materialized after the explicit `--materialize-large-files` flag.

```bash
python3 scripts/verify_manifest.py
python3 scripts/refresh.py --candidate glm-4.7
python3 scripts/refresh.py --candidate glm-4.7 --materialize-large-files
python3 scripts/preflight.py glm-4.7 --verify-files --files-root /path/to/Q4_K_M
```

A refresh does not automatically replace the research manifests. Review revision changes, licensing, templates, shard pointers, runtime source, and claims, then create a new dated package. Never rewrite an old package in place.

## Quantization option refresh

`refresh.py` also reads `manifests/quantization_options.json` and checks that every captured option directory exists in the exact detached converter revision. It does not query `main`, `latest`, tags, or model aliases. Rounded display sizes must be re-captured explicitly and reviewed; they are not inferred from directory names.
