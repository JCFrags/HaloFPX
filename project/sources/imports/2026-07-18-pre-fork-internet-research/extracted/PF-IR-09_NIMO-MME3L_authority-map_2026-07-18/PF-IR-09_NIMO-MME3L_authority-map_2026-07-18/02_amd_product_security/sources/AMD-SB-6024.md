# AMD-SB-6024 — AMD Graphics Driver Vulnerabilities, February 2026

- **Source ID:** `AMD-SB-6024`
- **Authority:** AMD Product Security
- **Canonical URL:** https://www.amd.com/en/resources/product-security/bulletin/amd-sb-6024.html
- **Access date:** 2026-07-18
- **Source type:** Official security bulletin
- **Applicability label:** [FAMILY_APPLIES] [FIX_AVAILABLE]
- **Confidence:** High for AMD family mapping; Medium for exact installed stack

## Decision-relevant official claims

- [OFFICIAL] Ryzen AI Max 300 integrated graphics map to the Radeon RX 7000 and Radeon PRO W7000 mitigation tables.
- [OFFICIAL] RX 7000 floors include Adrenalin 25.6.1 for CVE-2024-36324/CVE-2025-48518; Adrenalin 25.5.1 for CVE-2024-36316/CVE-2024-36320/CVE-2024-36319; Radeon Software for Linux 25.10.1 for CVE-2024-36319; Linux 25.10.2 for non-AMD CVE-2025-21940.
- [OFFICIAL] PRO W7000 uses PRO Edition 25.Q2 for listed AMD CVEs and Linux 25.10.1/25.10.2 where specified.

## Explicit gaps and limits

- [OPEN] Installed operating system, AMD driver branch, packaging source, and loaded firmware tuple.
- Linux distribution kernels may not use the Radeon Software for Linux package version scheme.

## License / reuse

Copyright AMD; normalized factual extract. No reuse license grant inferred.

## Capture note

This is an access-dated normalized factual capture, not a byte-for-byte server response. Exact identifiers, version floors, dates, and scope language are retained; page navigation and presentation are omitted.
