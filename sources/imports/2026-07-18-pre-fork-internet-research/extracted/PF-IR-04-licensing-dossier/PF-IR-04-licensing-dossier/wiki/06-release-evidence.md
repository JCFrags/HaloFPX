# Evidence required for a concrete source or binary release

## Artifact identity

Record every distributed file, archive, package, container layer, installer payload, model/data file, shared library, generated asset, documentation file, and license/notice file with path, size, SHA-256, media type, and origin.

## SBOM

Generate both source and binary SBOM views. At minimum include component name, supplier, exact version/commit, checksums, download/source locator, license declared, license concluded, copyright, relationship, and reviewer annotation. Use `NOASSERTION` rather than guessing.

This dossier's `release-sbom-seed.spdx.json` is only a seed for the public components.

## Source-to-binary mapping

Populate `manifests/source-to-binary-map.template.csv` for:

- each compiled object/archive/shared library;
- generated Web UI asset and generated header;
- statically embedded license/notice resources;
- scripts/configuration/service files installed beside binaries;
- model/tokenizer/template/corpus files included in packages.

## GPL corresponding-source evidence

For any release the human reviewer determines is subject to GPL object-code conveyance, preserve the preferred form for modification and the scripts/interface definitions/build controls required by the applicable terms. Record the chosen source-delivery method next to the binary offer/access point. Determine separately whether installation information is applicable.

## Notices

Ship exact license texts and attribution/NOTICE material required by each included file/package. Do not rely on a root MIT file to cover Apache, LLVM exception, BSD, ISC, Unlicense, public-domain, model, documentation, or package-specific records.

## Build provenance

Bind source commits, submodule pins, toolchain packages, build parameters, environment digest, outputs, and attestations. A reproducible build comparison is preferred; discrepancies must be explained.
