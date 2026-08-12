# amd64_edac source at commit 94515f3a

- **Source ID:** `KERNEL-SOURCE-AMD64-EDAC`
- **Authority:** Linux kernel upstream source
- **Canonical URL:** https://github.com/torvalds/linux/blob/94515f3a7d4256a5062176b7d6ed0471938cd51a/drivers/edac/amd64_edac.c
- **Access date:** 2026-07-18
- **Source type:** Official upstream source capture
- **Applicability label:** [UNKNOWN]
- **Confidence:** Medium

## Decision-relevant official claims

- [OFFICIAL] Source blob SHA is `c6aa69dbd9fb12a04353e8f100730734a6f9053d` at kernel commit `94515f3a7d4256a5062176b7d6ed0471938cd51a`.
- [OFFICIAL] The driver matches AMD family 0x1A generally and uses UMC operations for families >= 0x17.
- [OFFICIAL] Probe requires a memory instance and ECC enabled; forcing ECC is discouraged on newer systems and BIOS enablement is expected.

## Explicit gaps and limits

- [UNKNOWN] Exact target CPUID family/model/stepping and whether the generic Family 1Ah model path correctly decodes Strix Halo.
- [UNKNOWN] LPDDR5 ECC implementation and firmware exposure.

## License / reuse

Linux kernel source is GPL-2.0-only with documented exceptions; module declares GPL. Preserve license and copyright.

## Capture note

This is an access-dated normalized factual capture unless a raw artifact path is explicitly identified. It is not represented as a byte-for-byte server response.
