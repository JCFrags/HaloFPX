# Raw evidence capture policy

This directory preserves the **local evidence artifacts** used by PF-IR-06. Every artifact is hashed in `../manifests/raw-manifest.json` and carries a literal capture-kind label.

## Fidelity classes

- `normalized-source-excerpt-from-immutable-revision`: compact source-controlled excerpt. The upstream repository, exact commit/version, path and Git blob ID are pinned; the local text is not represented as the complete byte-for-byte upstream file.
- `official-web-receipt-with-short-excerpt` / `official-standards-receipt-with-short-excerpt`: canonical URI, version/date, access date and compact fact/excerpt. The local file is not raw HTML.
- `metadata-receipt`: release/tag/index facts retained as a normalized record.
- `non-target receipt`: compact Windows/macOS portability evidence only.

The exact bytes of **these local captures** are preserved by SHA-256. Full upstream tarballs and copyrighted standards pages are intentionally not embedded. For source-controlled material, the recorded Git blob ID is the independent upstream byte-identity anchor; retrieve the pinned repository object and compare it before expanding or refreshing an excerpt.

## Reproduction rule

A refresh must use a new dated bundle. Do not overwrite an earlier evidence set. Record the retrieval method, retain the previous manifest, and review every changed claim/source/test mapping before accepting the new bundle.
