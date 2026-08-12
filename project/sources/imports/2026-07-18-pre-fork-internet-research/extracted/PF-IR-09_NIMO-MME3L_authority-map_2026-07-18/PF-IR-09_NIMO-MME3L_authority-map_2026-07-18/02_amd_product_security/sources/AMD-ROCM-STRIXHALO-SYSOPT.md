# System optimization for Strix Halo

- **Source ID:** `AMD-ROCM-STRIXHALO-SYSOPT`
- **Authority:** AMD ROCm documentation
- **Canonical URL:** https://rocm.docs.amd.com/en/docs-7.2.0/how-to/system-optimization/strixhalo.html
- **Access date:** 2026-07-18
- **Source type:** Official technical documentation
- **Applicability label:** [EXPLICITLY_APPLIES]
- **Confidence:** High

## Decision-relevant official claims

- [OFFICIAL] AMD documents required KFD kernel fixes for Strix Halo and names commits `7f26af7` and `7445db6`.
- [OFFICIAL] The fixes are documented as included in Linux 6.18.4 and later.

## Explicit gaps and limits

- [OPEN] Running kernel version and downstream backport status.
- [HOLD] Compute/ROCm deployment should not be assumed sound below the documented floor without verified backports.

## License / reuse

AMD ROCm documentation is generally provided under its documented site/repository terms; preserve attribution and inspect the source license before redistribution beyond this evidence pack.

## Capture note

This is an access-dated normalized factual capture, not a byte-for-byte server response. Exact identifiers, version floors, dates, and scope language are retained; page navigation and presentation are omitted.
