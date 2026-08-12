# 14. Primary-source and manifest index

## Machine-readable indexes

- [`../manifests/sources.csv`](../manifests/sources.csv) — source IDs, commits/versions, URLs, PCI applicability, access dates, local captures, licenses, and blob hashes.
- [`../manifests/claims.csv`](../manifests/claims.csv) — literal labels, statements, evidence IDs, confidence, and decision impact.
- [`../manifests/support-boundary.csv`](../manifests/support-boundary.csv) — upstream/vendor/Windows/missing boundary by topic.
- [`../manifests/versions.json`](../manifests/versions.json) — target identity, current pins, and exact AMD package names.
- [`../manifests/licenses.csv`](../manifests/licenses.csv) — license/terms boundary and bundle handling.
- [`../manifests/access-log.csv`](../manifests/access-log.csv) — access chronology.
- [`../manifests/exact-source-verification.json`](../manifests/exact-source-verification.json) — Git blob verification.
- [`../manifests/files.sha256`](../manifests/files.sha256) — whole-bundle checksums.
- [`../manifests/validation.json`](../manifests/validation.json) — structural validation.

## Key local primary captures

### Linux

- [`../sources/raw/kernel/Kconfig`](../sources/raw/kernel/Kconfig)
- [`../sources/raw/kernel/npu5_regs.c`](../sources/raw/kernel/npu5_regs.c)
- [`../sources/raw/kernel/amdnpu.rst`](../sources/raw/kernel/amdnpu.rst)
- [`../sources/excerpts/kernel/amdxdna_pci_drv.c.lines26-139.txt`](../sources/excerpts/kernel/amdxdna_pci_drv.c.lines26-139.txt)
- [`../sources/excerpts/kernel/aie2_pci.c.suspend-resume-init.txt`](../sources/excerpts/kernel/aie2_pci.c.suspend-resume-init.txt)
- [`../sources/excerpts/kernel/amdxdna_gem.c.noncoherent-sync.txt`](../sources/excerpts/kernel/amdxdna_gem.c.noncoherent-sync.txt)
- [`../sources/excerpts/kernel/amdxdna_accel.h.qos-bo-sync-telemetry.txt`](../sources/excerpts/kernel/amdxdna_accel.h.qos-bo-sync-telemetry.txt)

### AMD XDNA driver/shim

- [`../sources/raw/amd-xdna-driver/legacy-npu5_regs.c`](../sources/raw/amd-xdna-driver/legacy-npu5_regs.c)
- [`../sources/raw/amd-xdna-driver/CMakeLists.txt`](../sources/raw/amd-xdna-driver/CMakeLists.txt)
- [`../sources/raw/amd-xdna-driver/.gitmodules`](../sources/raw/amd-xdna-driver/.gitmodules)
- [`../sources/records/amd-xdna-driver_README_relevant-capture.md`](../sources/records/amd-xdna-driver_README_relevant-capture.md)

### Firmware

- [`../sources/excerpts/vendor-docs/linux-firmware-WHENCE.amdxdna.txt`](../sources/excerpts/vendor-docs/linux-firmware-WHENCE.amdxdna.txt)

### Ryzen AI examples

- [`../sources/raw/ryzenai-sw/DistilBERT_text_classification_bf16_README.md`](../sources/raw/ryzenai-sw/DistilBERT_text_classification_bf16_README.md)
- [`../sources/raw/ryzenai-sw/run_inference.py`](../sources/raw/ryzenai-sw/run_inference.py)
- [`../sources/raw/ryzenai-sw/vitisai_config.json`](../sources/raw/ryzenai-sw/vitisai_config.json)
- [`../sources/raw/ryzenai-sw/custom_embedding.py`](../sources/raw/ryzenai-sw/custom_embedding.py)
- [`../sources/raw/ryzenai-sw/LICENSE.txt`](../sources/raw/ryzenai-sw/LICENSE.txt)

### Runtime/compiler licensing

- [`../sources/excerpts/runtime/XRT_LICENSE.lines1-55.txt`](../sources/excerpts/runtime/XRT_LICENSE.lines1-55.txt)
- [`../sources/excerpts/runtime/LLVM-AIE_LICENSE.lines1-20.txt`](../sources/excerpts/runtime/LLVM-AIE_LICENSE.lines1-20.txt)
- [`../sources/excerpts/runtime/MLIR-AIE_LICENSE.lines1-20.txt`](../sources/excerpts/runtime/MLIR-AIE_LICENSE.lines1-20.txt)

### Vendor documentation capture

- [`../sources/records/official-primary-claim-capture.md`](../sources/records/official-primary-claim-capture.md)
- [`../sources/CAPTURE_LIMITATIONS.md`](../sources/CAPTURE_LIMITATIONS.md)

## Re-fetch recipe

[`../sources/fetch-primary-sources.sh`](../sources/fetch-primary-sources.sh) records immutable raw URLs and expected Git blob IDs. It does not download AMD account-gated packages or firmware binaries.
