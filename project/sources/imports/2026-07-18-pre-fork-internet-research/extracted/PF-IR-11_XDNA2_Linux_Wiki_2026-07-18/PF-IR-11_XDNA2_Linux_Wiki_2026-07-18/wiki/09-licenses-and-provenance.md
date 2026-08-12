# 9. Licenses and provenance

## License boundary

| Component | Captured license/terms | Handling |
|---|---|---|
| Linux `amdxdna` | GPL-2.0 / GPL-2.0-only; UAPI has Linux syscall note | SPDX notices preserved |
| AMD legacy kernel source | GPL-2.0 | exact source preserved |
| AMD XDNA shim/build | Apache-2.0 | exact SPDX-bearing build/shim excerpts preserved |
| `linux-firmware` amdnpu | Redistributable; see `LICENSE.amdnpu` | binaries not included; reference preserved |
| XRT userspace | Apache-2.0 | public commit and license blob recorded |
| XRT kernel portions | GPL-2.0 or dual terms depending component | not used as target kernel proof |
| `llvm-aie` | Apache-2.0 with LLVM exception | public commit/license blob recorded |
| `mlir-aie` | Apache-2.0 with LLVM exception | public commit/license blob recorded |
| `amd/RyzenAI-SW` examples | MIT | exact license included and Git-blob verified |
| AMD Ryzen AI binaries | AMD EULA and third-party notices | packages absent; terms not interpreted |
| AMD web documentation | AMD website terms | factual claim capture only |

The machine-readable table is [`../manifests/licenses.csv`](../manifests/licenses.csv).

## Provenance rules

1. Every source has an access date.
2. Git sources use immutable commit pins.
3. Exact raw files record upstream Git blob SHA-1 where available.
4. Every bundle file receives SHA-256.
5. Vendor package filenames are preserved exactly.
6. Gated binaries and firmware payloads are not silently mirrored.
7. A public repository head is not asserted to be an AMD binary-package build input without evidence.
8. Structured vendor-document captures are distinguished from exact raw files.

## Exact source verification

[`../manifests/exact-source-verification.json`](../manifests/exact-source-verification.json) recomputes Git blob object IDs for exact captured Git files.

[`../manifests/files.sha256`](../manifests/files.sha256) covers the complete bundle.

## Capture limitations

See [`../sources/CAPTURE_LIMITATIONS.md`](../sources/CAPTURE_LIMITATIONS.md). Notably:

[MISSING] AMD account-gated package payloads and their hashes are absent.

[MISSING] Firmware binaries are absent.

[UNKNOWN] The relationship between public source heads and AMD's supported binary release is not proven.

[DECISION] These gaps are preserved as decision blockers rather than filled with assumptions.
