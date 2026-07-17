# Machine-readable schemas

The suite uses JSON Schema Draft 2020-12.

| Schema | Purpose |
|---|---|
| `case.schema.json` | One row in the conformance matrix |
| `observation.schema.json` | Raw result from one fork/build/lane |
| `reference-record.schema.json` | Immutable record promoted as a comparison oracle |
| `tolerance-profile.schema.json` | Scoped numeric or distributional gate |
| `fixture-manifest.schema.json` | Included, downloaded, generated, or operator-supplied fixtures |

Provenance is part of correctness. A numeric array without model, build, device, driver, runtime, and fixture digests is not a valid cross-fork reference.
