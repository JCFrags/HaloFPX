# Evidence and claim method

## Source hierarchy

1. Current AMD ROCm compatibility matrix for official device/OS/kernel support.
2. AMD release/install/transition documentation for maturity, packaging and removal.
3. AMD source repositories and exact tags/commits/gitlinks for build provenance.
4. Linux kernel, Ubuntu kernel-team/archive and Mesa project records for kernel/graphics facts.
5. User-supplied local tuple only for comparison.

## Literal claim labels

| Label | Meaning |
|---|---|
| `[DOCUMENTED_SUPPORT]` | directly stated in an authoritative support/release source |
| `[PREVIEW_AVAILABILITY]` | available but explicitly preview/preliminary |
| `[KNOWN_ISSUE]` | documented defect, limitation or workaround |
| `[UNVERIFIED_COMBINATION]` | combination not qualified by the captured sources |
| `[LOCAL_COMPARISON_ONLY]` | local observation; no official support inference |
| `[PROVENANCE_GAP]` | required identity/integrity evidence not located |
| `[MATURITY_CONFLICT]` | authoritative sources describe different support/maturity dimensions |
| `[NOT_RELEASE_READY]` | source project status lacks release-ready designation |
| `[SIGNED_REPOSITORY_METADATA]` | package metadata is verified through a documented signature mechanism |
| `[UNVERIFIED_SIGNATURE_STATUS]` | signature verification was not completed |
| `[SOURCE_PIN]` | exact source commit/tag/hash captured |
| `[LANE_BOUNDARY]` / `[DO_NOT_MIX]` | enforced separation rule |
| `[SBOM_NOT_LOCATED]` | no vendor SBOM located in captured material |
| `[PLAUSIBLE_SEPARATE_CANDIDATE]` | technically plausible, separately testable overlay |
| `[DOCUMENTATION_DRIFT]` | official documents differ; governing source is identified |

## Raw captures

The `raw/` tree contains structured capture records with exact URLs, access dates, source references, line locators, hashes and identifiers. It does not mirror full third-party web pages. Source manifests and lockfiles are retained in `manifests/`.

## Uncertainty policy

Unknown values are `null`, `not located`, or `unverified`; they are never filled by inference. In particular, the stable 7.14 tarball checksum/signature is deliberately unresolved. Promotion gates fail closed on those values.

## Revalidation

This dossier is date-scoped. Re-run source capture when AMD changes the matrix, publishes a 7.14.x patch, Linux 7.2 becomes stable, or repository metadata/artifacts change. Do not mutate this folder; create a new dated capture and compare manifests.
