# fwupd OnlyTrusted and Jcat trust semantics

- **Source ID:** `FWUPD-ONLYTRUSTED`
- **Authority:** fwupd project documentation
- **Canonical URL:** https://fwupd.github.io/libfwupdplugin/only-trusted.html
- **Access date:** 2026-07-18
- **Source type:** Official project documentation
- **Applicability label:** [OFFICIAL]
- **Confidence:** High

## Decision-relevant official claims

- [OFFICIAL] LVFS leaves the original firmware payload unmodified and uses Jcat detached checksums and PKCS#7 signatures.
- [OFFICIAL] fwupd trust answers whether an update came from a trusted source; device-side vendor-payload verification is a separate mechanism that may be implemented by the device.
- [OFFICIAL] Missing or untrusted firmware signatures are rejected when `OnlyTrusted` policy is enforced.

## Explicit gaps and limits

- [SIGNATURE_UNPROVEN] No exact NIMO BIOS, Crucial P310, or USB4 payload/device signature chain was located.
- Repository authenticity does not establish payload authorization by target firmware.

## License / reuse

fwupd project documentation; source examples carry LGPL-2.1-or-later notices. Retain attribution and license notices.

## Capture note

This is an access-dated normalized factual capture unless a raw artifact path is explicitly identified. It is not represented as a byte-for-byte server response.
