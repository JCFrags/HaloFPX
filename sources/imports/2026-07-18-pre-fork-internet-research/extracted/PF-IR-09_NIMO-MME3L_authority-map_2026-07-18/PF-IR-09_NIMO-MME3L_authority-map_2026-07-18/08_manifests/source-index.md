# Official source index

Access date: **2026-07-18**. Total normalized captures: **35**.

## 01_oem_nimo

| Source ID | Authority | Title | Applicability | Capture |
|---|---|---|---|---|
| `OEM-NIMO-PRODUCT-AI395` | NIMO official product page | NIMO AMD Ryzen AI MAX+ 395 Mini PC product page | [FAMILY_APPLIES] | [OEM-NIMO-PRODUCT-AI395.md](../01_oem_nimo/sources/OEM-NIMO-PRODUCT-AI395.md) |
| `OEM-NIMO-SUPPORT-AI395` | NIMO official support | Nimo AI 395 Minipc Manual & Driver Downloads | [FAMILY_APPLIES] | [OEM-NIMO-SUPPORT-AI395.md](../01_oem_nimo/sources/OEM-NIMO-SUPPORT-AI395.md) |

## 02_amd_product_security

| Source ID | Authority | Title | Applicability | Capture |
|---|---|---|---|---|
| `AMD-PSIRT-SCOPE` | AMD Product Security | AMD Product Security bulletin scope and disclosure boundary | [OFFICIAL] [DISCLOSURE_LIMITED] | [AMD-PSIRT-SCOPE.md](../02_amd_product_security/sources/AMD-PSIRT-SCOPE.md) |
| `AMD-ROCM-GFX1151-MATRIX` | AMD ROCm documentation | Ryzen AI Max graphics target mapping | [EXPLICITLY_APPLIES] | [AMD-ROCM-GFX1151-MATRIX.md](../02_amd_product_security/sources/AMD-ROCM-GFX1151-MATRIX.md) |
| `AMD-ROCM-STRIXHALO-SYSOPT` | AMD ROCm documentation | System optimization for Strix Halo | [EXPLICITLY_APPLIES] | [AMD-ROCM-STRIXHALO-SYSOPT.md](../02_amd_product_security/sources/AMD-ROCM-STRIXHALO-SYSOPT.md) |
| `AMD-SB-4013` | AMD Product Security | AMD-SB-4013 — AMD Athlon and Ryzen Processor Vulnerabilities, February 2026 | [EXPLICITLY_APPLIES] [FIX_AVAILABLE] | [AMD-SB-4013.md](../02_amd_product_security/sources/AMD-SB-4013.md) |
| `AMD-SB-4015` | AMD Product Security | AMD-SB-4015 — AMD Chipset Driver Vulnerabilities | [EXPLICITLY_APPLIES] [FIX_AVAILABLE] | [AMD-SB-4015.md](../02_amd_product_security/sources/AMD-SB-4015.md) |
| `AMD-SB-4016` | AMD Product Security | AMD-SB-4016 — AMD RAID software vulnerabilities | [EXPLICITLY_APPLIES] [FIX_AVAILABLE] | [AMD-SB-4016.md](../02_amd_product_security/sources/AMD-SB-4016.md) |
| `AMD-SB-4017` | AMD Product Security | AMD-SB-4017 — Ryzen Processor Vulnerabilities, May 2026 | [EXPLICITLY_APPLIES] [FIX_AVAILABLE] | [AMD-SB-4017.md](../02_amd_product_security/sources/AMD-SB-4017.md) |
| `AMD-SB-6010` | AMD Product Security | AMD-SB-6010 — GPU memory information disclosure | [FAMILY_APPLIES] [FIX_AVAILABLE] | [AMD-SB-6010.md](../02_amd_product_security/sources/AMD-SB-6010.md) |
| `AMD-SB-6013` | AMD Product Security | AMD-SB-6013 — GPU register information disclosure | [FAMILY_APPLIES] [FIX_AVAILABLE] | [AMD-SB-6013.md](../02_amd_product_security/sources/AMD-SB-6013.md) |
| `AMD-SB-6024` | AMD Product Security | AMD-SB-6024 — AMD Graphics Driver Vulnerabilities, February 2026 | [FAMILY_APPLIES] [FIX_AVAILABLE] | [AMD-SB-6024.md](../02_amd_product_security/sources/AMD-SB-6024.md) |
| `AMD-SB-6027` | AMD Product Security | AMD-SB-6027 — AMD Graphics Driver Vulnerability | [EXPLICITLY_APPLIES] [FIX_AVAILABLE] | [AMD-SB-6027.md](../02_amd_product_security/sources/AMD-SB-6027.md) |
| `AMD-SB-7033` | AMD Product Security | AMD-SB-7033 — CPU Microcode Signature Verification Vulnerability | [EXPLICITLY_APPLIES] [FIX_AVAILABLE] | [AMD-SB-7033.md](../02_amd_product_security/sources/AMD-SB-7033.md) |
| `AMD-SB-7048` | AMD Product Security | AMD-SB-7048 — Phoenix: Rowhammer Attacks on DDR5 Memory | [EXPLICITLY_APPLIES] | [AMD-SB-7048.md](../02_amd_product_security/sources/AMD-SB-7048.md) |
| `AMD-SB-7054` | AMD Product Security | AMD-SB-7054 — Incorrect EFI LocateProtocol use in SMI handler | [EXPLICITLY_APPLIES] [FIX_AVAILABLE] | [AMD-SB-7054.md](../02_amd_product_security/sources/AMD-SB-7054.md) |
| `AMD-SB-7055` | AMD Product Security | AMD-SB-7055 — RDSEED Failure on AMD Zen 5 Processors | [EXPLICITLY_APPLIES] [FIX_AVAILABLE] | [AMD-SB-7055.md](../02_amd_product_security/sources/AMD-SB-7055.md) |
| `AMD-SB-7056` | AMD Product Security | AMD-SB-7056 — PCIe specification-related issue | [NOT_LISTED_FOR_TARGET] | [AMD-SB-7056.md](../02_amd_product_security/sources/AMD-SB-7056.md) |

## 03_firmware_supply

| Source ID | Authority | Title | Applicability | Capture |
|---|---|---|---|---|
| `FWUPD-ONLYTRUSTED` | fwupd project documentation | fwupd OnlyTrusted and Jcat trust semantics | [OFFICIAL] | [FWUPD-ONLYTRUSTED.md](../03_firmware_supply/sources/FWUPD-ONLYTRUSTED.md) |
| `FWUPD-VERSION-GATES` | fwupd project documentation | fwupd device GUID, minimum version, and bootloader gates | [OFFICIAL] | [FWUPD-VERSION-GATES.md](../03_firmware_supply/sources/FWUPD-VERSION-GATES.md) |
| `LINUX-FIRMWARE-MAIN` | linux-firmware upstream | linux-firmware main branch snapshot | [FAMILY_APPLIES] | [LINUX-FIRMWARE-MAIN.md](../03_firmware_supply/sources/LINUX-FIRMWARE-MAIN.md) |
| `LINUX-FIRMWARE-WHENCE-AMD` | linux-firmware upstream | linux-firmware WHENCE and AMDGPU license/provenance | [FAMILY_APPLIES] | [LINUX-FIRMWARE-WHENCE-AMD.md](../03_firmware_supply/sources/LINUX-FIRMWARE-WHENCE-AMD.md) |
| `LVFS-PULP-MANIFEST` | Linux Vendor Firmware Service | LVFS public PULP_MANIFEST and literal target search | [OFFICIAL] [OPEN] | [LVFS-PULP-MANIFEST.md](../03_firmware_supply/sources/LVFS-PULP-MANIFEST.md) |
| `LVFS-UPLOAD-SIGNING` | LVFS documentation | LVFS upload, detached signing, and remote visibility model | [OFFICIAL] | [LVFS-UPLOAD-SIGNING.md](../03_firmware_supply/sources/LVFS-UPLOAD-SIGNING.md) |

## 04_storage_crucial_micron

| Source ID | Authority | Title | Applicability | Capture |
|---|---|---|---|---|
| `CRUCIAL-P310-PRODUCT-FLYER` | Crucial / Micron | Crucial P310 2230 product flyer | [FAMILY_APPLIES] | [CRUCIAL-P310-PRODUCT-FLYER.md](../04_storage_crucial_micron/sources/CRUCIAL-P310-PRODUCT-FLYER.md) |
| `CRUCIAL-P310-SUPPORT` | Crucial official support | Crucial P310 SSD support page | [FAMILY_APPLIES] [NO_PUBLIC_PACKAGE] | [CRUCIAL-P310-SUPPORT.md](../04_storage_crucial_micron/sources/CRUCIAL-P310-SUPPORT.md) |
| `CRUCIAL-STORAGE-EXECUTIVE-FAQ` | Crucial official support | Crucial Storage Executive FAQ | [FAMILY_APPLIES] | [CRUCIAL-STORAGE-EXECUTIVE-FAQ.md](../04_storage_crucial_micron/sources/CRUCIAL-STORAGE-EXECUTIVE-FAQ.md) |

## 05_kernel_ras

| Source ID | Authority | Title | Applicability | Capture |
|---|---|---|---|---|
| `KERNEL-AMDGPU-RAS-DOC` | Linux kernel documentation | AMDGPU RAS support interfaces | [SUPPORTED_IF_PRESENT] | [KERNEL-AMDGPU-RAS-DOC.md](../05_kernel_ras/sources/KERNEL-AMDGPU-RAS-DOC.md) |
| `KERNEL-EDAC-DOC` | Linux kernel documentation | Error Detection and Correction devices | [SUPPORTED_IF_PRESENT] | [KERNEL-EDAC-DOC.md](../05_kernel_ras/sources/KERNEL-EDAC-DOC.md) |
| `KERNEL-PCIE-AER` | Linux kernel documentation | PCI Express Advanced Error Reporting driver guide | [SUPPORTED_IF_PRESENT] | [KERNEL-PCIE-AER.md](../05_kernel_ras/sources/KERNEL-PCIE-AER.md) |
| `KERNEL-SOURCE-AMD64-EDAC` | Linux kernel upstream source | amd64_edac source at commit 94515f3a | [UNKNOWN] | [KERNEL-SOURCE-AMD64-EDAC.md](../05_kernel_ras/sources/KERNEL-SOURCE-AMD64-EDAC.md) |
| `KERNEL-SOURCE-AMDGPU-DISCOVERY` | Linux kernel upstream source | amdgpu IP discovery source at commit 94515f3a | [FAMILY_APPLIES] | [KERNEL-SOURCE-AMDGPU-DISCOVERY.md](../05_kernel_ras/sources/KERNEL-SOURCE-AMDGPU-DISCOVERY.md) |
| `KERNEL-SOURCE-AMDGPU-RAS` | Linux kernel upstream source | amdgpu_ras source at commit 94515f3a | [UNKNOWN] | [KERNEL-SOURCE-AMDGPU-RAS.md](../05_kernel_ras/sources/KERNEL-SOURCE-AMDGPU-RAS.md) |
| `KERNEL-USB4-THUNDERBOLT` | Linux kernel documentation | USB4 and Thunderbolt security and NVM update authority | [SUPPORTED_IF_PRESENT] | [KERNEL-USB4-THUNDERBOLT.md](../05_kernel_ras/sources/KERNEL-USB4-THUNDERBOLT.md) |
| `KERNEL-X86-MCA` | Linux kernel documentation | x86-64 machine-check interface | [SUPPORTED_IF_PRESENT] | [KERNEL-X86-MCA.md](../05_kernel_ras/sources/KERNEL-X86-MCA.md) |

