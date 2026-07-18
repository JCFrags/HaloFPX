# fwupd device GUID, minimum version, and bootloader gates

- **Source ID:** `FWUPD-VERSION-GATES`
- **Authority:** fwupd project documentation
- **Canonical URL:** https://fwupd.github.io/libfwupdplugin/tutorial.html
- **Access date:** 2026-07-18
- **Source type:** Official project documentation
- **Applicability label:** [OFFICIAL]
- **Confidence:** High

## Decision-relevant official claims

- [OFFICIAL] A device GUID/instance ID must match metadata for an update to succeed.
- [OFFICIAL] Plugins can expose a lowest allowed firmware version and a bootloader version, allowing client-side downgrade or signed-firmware gates.
- [OFFICIAL] Update eligibility is device/plugin-specific.

## Explicit gaps and limits

- [ROLLBACK_UNPROVEN] Client-side minimum-version policy is not the same as hardware-enforced anti-rollback.
- [OPEN] Target fwupd GUIDs, flags, lowest version, and bootloader version.

## License / reuse

fwupd project documentation and examples include LGPL-2.1-or-later notices; retain attribution and license notices.

## Capture note

This is an access-dated normalized factual capture unless a raw artifact path is explicitly identified. It is not represented as a byte-for-byte server response.
