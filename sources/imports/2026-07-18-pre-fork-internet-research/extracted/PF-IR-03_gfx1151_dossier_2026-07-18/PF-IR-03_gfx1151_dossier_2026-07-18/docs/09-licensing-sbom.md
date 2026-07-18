# Licenses, notices and SBOM

## Status

[LICENSE_INVENTORY_PARTIAL_UNTIL_ARTIFACT_SCAN] The source/build roots and package declarations identify many licenses, but the stable gfx1151 artifact bytes were not available for a complete file-level inventory. The table in [licenses.csv](../manifests/licenses.csv) distinguishes verified declarations from mandatory acquisition-time scans.

## Confirmed or declared categories

- TheRock build system: MIT.
- LLVM/Clang: Apache-2.0 with LLVM exception.
- Most ROCm runtime/math packages, including RCCL package metadata: MIT declarations, subject to per-component/imported-code notices.
- Linux kernel: GPL-2.0-only.
- Mesa/RADV: mixed permissive licenses, predominantly MIT/X11-family; exact files control.
- `linux-firmware`: mixed per-file licensing; `WHENCE` and per-blob notices are mandatory.
- Bundled system dependencies: multiple licenses; package metadata's aggregate `MIT` field is not sufficient to inventory them.

## Required artifact scan

Run:

```bash
scripts/inventory-artifact.sh <extracted-sdk-or-rootfs> <output-directory>
scripts/generate-sbom.sh <extracted-sdk-or-rootfs> <output-directory>
```

Review every match for `LICENSE*`, `COPYING*`, `NOTICE*`, package copyright metadata and SPDX headers. Retain the raw files, normalized inventory, file hashes and tool versions. A generated SBOM is evidence produced by the build process, not an AMD-issued SBOM.

## Notice handling

`NOTICE.md` in this folder describes the dossier's own content. It does not grant redistribution rights for AMD binaries or third-party code. Redistributors must follow each acquired component's license and notice requirements.
