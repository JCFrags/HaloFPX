# USB4 and Thunderbolt security and NVM update authority

- **Source ID:** `KERNEL-USB4-THUNDERBOLT`
- **Authority:** Linux kernel documentation
- **Canonical URL:** https://docs.kernel.org/admin-guide/thunderbolt.html
- **Access date:** 2026-07-18
- **Source type:** Official upstream documentation
- **Applicability label:** [SUPPORTED_IF_PRESENT]
- **Confidence:** High for interface; Low for exact controller

## Decision-relevant official claims

- [OFFICIAL] Kernel documentation covers device authorization/security levels, IOMMU DMA protection, sysfs identity, NVM version, host/device/retimer update paths, and safe mode.
- [OFFICIAL] fwupd/LVFS is preferred; manual update writes a suitable image to non-active NVM and triggers `nvm_authenticate`.
- [OFFICIAL] A non-zero authentication status indicates failure; retimer servicing may use `offline` and `rescan` where supported.
- [OFFICIAL] The documentation warns that an unsuitable update may render hardware unusable without special tools.

## Explicit gaps and limits

- [OPEN: LOCAL_ID_REQUIRED] Exact host router/controller/retimer vendor, device, generation, NVM version, security level, IOMMU status, and OEM image.
- [ROLLBACK_UNPROVEN] Non-active NVM is not proof that rollback is supported.
- No documented generic CE/UE link-error counter equivalent to AER for every USB4 topology.

## License / reuse

Linux kernel documentation; preserve attribution and source license.

## Capture note

This is an access-dated normalized factual capture unless a raw artifact path is explicitly identified. It is not represented as a byte-for-byte server response.
