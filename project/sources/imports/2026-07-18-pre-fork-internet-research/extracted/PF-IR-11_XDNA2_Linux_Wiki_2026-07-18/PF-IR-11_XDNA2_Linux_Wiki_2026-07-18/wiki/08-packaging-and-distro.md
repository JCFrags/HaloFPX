# 8. Packaging and target-distro boundary

## AMD reference environment

[VENDOR-ONLY] The captured Ryzen AI Software 1.7.1 Linux reference specifies:

| Item | Reference |
|---|---|
| Distribution | Ubuntu 24.04 LTS |
| Kernel | 6.10 or newer |
| Python | 3.12.x |
| Memory guidance | 64 GB recommended |
| XRT base | `xrt_202610.2.21.75_24.04-amd64-base.deb` |
| XRT development | `xrt_202610.2.21.75_24.04-amd64-base-dev.deb` |
| XRT NPU | `xrt_202610.2.21.75_24.04-amd64-npu.deb` |
| XDNA plugin | `xrt_plugin.2.21.260102.53.release_24.04-amd64-amdxdna.deb` |
| Ryzen AI archive | `ryzen_ai-1.7.1.tgz` |

[VENDOR-ONLY] The install path is account-gated and EULA-governed.

[MISSING] Package hashes and package contents were not captured because the gated assets were not downloaded.

## Distro kernel versus vendor module

[UPSTREAM] A sufficiently new distro kernel may contain `amdxdna`.

[VENDOR-ONLY] AMD's source repository says Ubuntu 25.04 includes an in-tree driver but still needs the XRT shim.

[VENDOR-ONLY] AMD's plugin packaging can install a DKMS-built staging driver and a separate legacy driver.

[VENDOR-ONLY] AMD warns that an older distro driver can enumerate and execute basic paths while failing newer plugin ioctls.

[DECISION] Record the loaded module filename, build ID, signer, package owner, UAPI behavior, and firmware lineage. Do not treat “in-tree” or “vendor” as automatically superior; treat a matched, reproducible stack as the requirement.

## Source-build boundary

[VENDOR-ONLY] AMD's source instructions support Ubuntu/Debian and Arch-oriented packaging, but this does not make every rolling distro a supported high-level Ryzen AI Software target.

[VENDOR-ONLY] AMD recommends matching XRT base to the plugin source, commonly through the repository submodule.

[UNKNOWN] Rebuilding public sources does not guarantee bit-for-bit equivalence to AMD's 1.7.1 packages or access to all high-level compiler/model assets.

## Target-distro classification

Until probed, classify the target as:

| Layer | State |
|---|---|
| PCI identity | `[UNKNOWN]` |
| kernel feature | `[UNKNOWN]` |
| IOMMU/SVA/PASID | `[UNKNOWN]` |
| firmware | `[UNKNOWN]` |
| XRT/plugin | `[UNKNOWN]` |
| Ryzen AI high-level runtime | `[UNKNOWN]` |
| package reproducibility | `[MISSING]` |
| model compilation | `[MISSING]` |
| performance/reliability | `[MISSING]` |

The probe produces evidence but does not install or fix anything.
