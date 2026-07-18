# PCI Express Advanced Error Reporting driver guide

- **Source ID:** `KERNEL-PCIE-AER`
- **Authority:** Linux kernel documentation
- **Canonical URL:** https://docs.kernel.org/PCI/pcieaer-howto.html
- **Access date:** 2026-07-18
- **Source type:** Official upstream documentation
- **Applicability label:** [SUPPORTED_IF_PRESENT]
- **Confidence:** High for interface; Medium for target capability

## Decision-relevant official claims

- [OFFICIAL] AER attaches only to Root Ports/RCECs that implement AER capability.
- [OFFICIAL] `CONFIG_PCIEAER` depends on `CONFIG_PCIEPORTBUS`; Linux handles AER only when firmware grants control through ACPI `_OSC`.
- [OFFICIAL] Correctable and uncorrectable statistics can be exposed in sysfs when events are captured.
- [OFFICIAL] AER covers PCIe hierarchy/link errors and excludes device-specific internal errors.

## Explicit gaps and limits

- [OPEN] Target AER capability, `_OSC` ownership, masks, counters, DPC, and driver recovery callbacks.
- No conclusion about internal GPU/NVMe/USB4 controller ECC coverage.

## License / reuse

Linux kernel documentation; source documentation follows kernel licensing terms. Preserve copyright and attribution.

## Capture note

This is an access-dated normalized factual capture unless a raw artifact path is explicitly identified. It is not represented as a byte-for-byte server response.
