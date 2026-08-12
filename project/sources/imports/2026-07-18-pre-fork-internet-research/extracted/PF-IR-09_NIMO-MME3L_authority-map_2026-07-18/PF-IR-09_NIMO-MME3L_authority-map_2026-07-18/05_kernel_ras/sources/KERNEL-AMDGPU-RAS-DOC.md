# AMDGPU RAS support interfaces

- **Source ID:** `KERNEL-AMDGPU-RAS-DOC`
- **Authority:** Linux kernel documentation
- **Canonical URL:** https://docs.kernel.org/gpu/amdgpu/ras.html
- **Access date:** 2026-07-18
- **Source type:** Official upstream documentation
- **Applicability label:** [SUPPORTED_IF_PRESENT]
- **Confidence:** High for interface; Low for target block support

## Decision-relevant official claims

- [OFFICIAL] AMDGPU exposes informational RAS queries in sysfs and injection controls in debugfs.
- [OFFICIAL] Error classes include `ue`, `ce`, and `poison`; per-block files report corrected and uncorrected counts.
- [OFFICIAL] Operations are allowed only for supported blocks; users must inspect `ras_mask` and per-device `ras/features`.
- [OFFICIAL] Unrecoverable-error handling can include GPU reset or optional system reboot.

## Explicit gaps and limits

- [UNKNOWN] Whether gfx1151/PCI 1002:1586 rev c1 exposes any supported RAS blocks.
- Absence of sysfs counters may mean unsupported/gated capability, not absence of faults.

## License / reuse

Linux kernel documentation; preserve attribution and source license.

## Capture note

This is an access-dated normalized factual capture unless a raw artifact path is explicitly identified. It is not represented as a byte-for-byte server response.
