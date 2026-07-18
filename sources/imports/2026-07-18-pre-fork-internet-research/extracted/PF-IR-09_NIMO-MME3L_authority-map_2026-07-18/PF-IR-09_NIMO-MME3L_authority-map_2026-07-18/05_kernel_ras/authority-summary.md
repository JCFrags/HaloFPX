# Kernel, PCIe/AER, EDAC/MCA, AMDGPU RAS, and USB4 authority map

## Classification rule

Kernel documentation describes interfaces; hardware and firmware decide whether the interfaces exist and which events reach the OS. All target-specific support remains conditional until the local collector demonstrates it.

## PCIe AER

`[SUPPORTED_IF_PRESENT]`. Requires AER-capable Root Ports/RCECs, `CONFIG_PCIEAER`, PCIe port-bus support, and ACPI `_OSC` ownership granted to Linux. It can report correctable, uncorrectable non-fatal, and fatal hierarchy/link errors and expose counters. It does not cover device-internal errors.

## CPU MCA

`[SUPPORTED_IF_PRESENT] [UNKNOWN for exact banks]`. Corrected events may be logged; uncorrected events may produce MCE/SIGBUS/panic/reboot. Bank meanings and firmware-first routing are CPU-specific. Capture CPUID, APEI/GHES, rasdaemon/trace and persistent logs.

## Memory EDAC

`[UNKNOWN]` for Strix Halo system memory. EDAC defines corrected, uncorrected, deferred, fatal and informational types, but the target must expose a memory-controller instance with ECC enabled. Upstream `amd64_edac` contains Family 1Ah support machinery but does not constitute a public guarantee for this exact LPDDR5 platform and board.

## AMDGPU RAS

`[UNKNOWN]`. Generic interfaces support per-block `ce`, `ue`, poison, features, page retirement and reset/reboot behavior only for supported blocks. The source gate is based on the complete discovered IP tuple and PSP/VBIOS capability, not `gfx1151` alone. At the captured kernel revision, MP0 14.0.3 is in the IP-discovery whitelist and 14.0.2 is not; the target MP0 version is `[OPEN]`.

## USB4 / Thunderbolt

`[OPEN: LOCAL_ID_REQUIRED]`. Kernel authorities expose identity, security level, authorization, IOMMU DMA protection, NVM version/authentication and retimer update mechanisms. Exact errata and firmware applicability are controller/OEM-specific. Non-active NVM is a staging mechanism, not proof of rollback. No firmware write is authorized without exact IDs, image match, authentication and recovery proof.

## Operational telemetry is not hardware-RAS proof

GPU resets, NVMe errors, USB4 disconnects and PCIe AER events are useful symptoms but do not establish complete corrected/uncorrected coverage or protection against silent data corruption. The integrity-control matrix identifies compensating detection layers.
