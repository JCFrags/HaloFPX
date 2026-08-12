# Utility scripts

These scripts generate or lock fixtures; they do not define expected model results.

- `generate-long-context.py` uses the tested `/tokenize` endpoint to target token counts.
- `generate-passkey.py` inserts a reproducible exact nonce.
- `generate-pathological-template.py` creates bounded template-depth probes.
- `build-fixture-manifest.py` hashes locally materialized models and generated fixtures.
- `record-reference.py` wraps a raw observation in a proposed reference record.
- `calibrate-tolerances.py` reports observed deltas but leaves normative limits unset.
- `verify-suite.py` validates matrix, manifest, references, and the no-invented-numeric-results policy.
