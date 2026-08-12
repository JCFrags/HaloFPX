# AMD-SB-7055 — RDSEED Failure on AMD Zen 5 Processors

- **Source ID:** `AMD-SB-7055`
- **Authority:** AMD Product Security
- **Canonical URL:** https://www.amd.com/en/resources/product-security/bulletin/amd-sb-7055.html
- **Access date:** 2026-07-18
- **Source type:** Official security bulletin
- **Applicability label:** [EXPLICITLY_APPLIES] [FIX_AVAILABLE]
- **Confidence:** High

## Decision-relevant official claims

- [EXPLICITLY_APPLIES] `AMD Ryzen AI Max 300 Series Processors` are listed.
- [OFFICIAL] CVE-2025-62626 mitigation is `StrixHaloPI-FP11_1.0.0.2a`, release 2025-11-25.
- [OFFICIAL] Bulletin distinguishes affected 16/32-bit RDSEED behavior from unaffected 64-bit RDSEED and documents software workarounds such as treating a zero result as failure.

## Explicit gaps and limits

- [HOLD] BIOS date precedes mitigation release.
- [OPEN] Target workload use of affected instruction forms.

## License / reuse

Copyright AMD; normalized factual extract. No reuse license grant inferred.

## Capture note

This is an access-dated normalized factual capture, not a byte-for-byte server response. Exact identifiers, version floors, dates, and scope language are retained; page navigation and presentation are omitted.
