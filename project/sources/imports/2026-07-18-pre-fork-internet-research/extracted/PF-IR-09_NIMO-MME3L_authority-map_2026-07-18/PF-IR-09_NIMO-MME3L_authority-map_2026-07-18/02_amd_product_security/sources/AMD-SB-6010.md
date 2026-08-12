# AMD-SB-6010 — GPU memory information disclosure

- **Source ID:** `AMD-SB-6010`
- **Authority:** AMD Product Security
- **Canonical URL:** https://www.amd.com/en/resources/product-security/bulletin/amd-sb-6010.html
- **Access date:** 2026-07-18
- **Source type:** Official security bulletin
- **Applicability label:** [FAMILY_APPLIES] [FIX_AVAILABLE]
- **Confidence:** Medium

## Decision-relevant official claims

- [OFFICIAL] CVE-2023-4969 is mitigated through an isolation mode that clears relevant state and prevents parallel process use; the mode is not enabled by default and can affect performance.
- [OFFICIAL] Ryzen AI Max 300 maps to RX 7000 / PRO W7000 guidance; captured current floors are Adrenalin 26.1.1 / PRO 26.Q1.

## Explicit gaps and limits

- [OPEN] Whether the isolation mode is enabled and acceptable for the deployment.
- [OPEN] Linux-equivalent operational control for the chosen stack.

## License / reuse

Copyright AMD; normalized factual extract. No reuse license grant inferred.

## Capture note

This is an access-dated normalized factual capture, not a byte-for-byte server response. Exact identifiers, version floors, dates, and scope language are retained; page navigation and presentation are omitted.
