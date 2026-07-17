# KV-cache validation suite

This standard-library-only suite validates the outer storage contracts documented in the Wiki. It does not attempt to deserialize opaque model-engine state.

```bash
./run_all.sh
```

The suite generates deterministic fixtures, verifies explicit LP64/little-endian CachyLLama layouts, validates the proposed HaloFPX immutable-object and manifest binding, runs unit tests, injects controlled corruptions, and computes SSD-endurance scenarios.

The checked-in result set records 9 passing unit tests and 18 passing fault-injection cases. `validation-summary.json` also asserts that no offline path emitted public hit eligibility.

## Status model

- `CATALOG_ENTRY_VALID`: a keyed-authenticated manifest and immutable object are internally valid, but one or more authorized current-request bindings were not supplied; it is not eligible for engine import.
- `IMPORT_CANDIDATE_VALID`: a keyed-authenticated HaloFPX manifest and its immutable object passed offline outer validation and may be offered to an isolated engine importer. It is not a public hit.
- `OBJECT_VALID`: an immutable object passed unkeyed digest and structure checks but has not yet been authenticated by the catalog, authorized, or engine-imported.
- `LEGACY_STRUCTURALLY_VALID_UNAUTHENTICATED`: a legacy file is bounded and structurally consistent, but lacks payload integrity and is not eligible for a trusted hit.
- `MISS_RECOMPUTE`: invalid, incompatible, incomplete, unavailable, unsupported, or corrupt state. The caller must ignore it and recompute.

Principal authorization and engine import remain additional fail-closed boundaries. Any import rejection, exception, impossible cursor, or post-import invariant violation converts the candidate to `MISS_RECOMPUTE` and the destination context must be discarded.

## Test-key warning

`generate_fixtures.py` writes a deterministic HMAC key under `fixtures/halofpx/keys/` solely for executable test vectors. It is public test material, not a production secret. Production implementations should obtain catalog-authentication keys from a protected key service or file descriptor rather than command-line arguments or cache storage.
