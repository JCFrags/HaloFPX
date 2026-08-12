# amdgpu IP discovery source at commit 94515f3a

- **Source ID:** `KERNEL-SOURCE-AMDGPU-DISCOVERY`
- **Authority:** Linux kernel upstream source
- **Canonical URL:** https://github.com/torvalds/linux/blob/94515f3a7d4256a5062176b7d6ed0471938cd51a/drivers/gpu/drm/amd/amdgpu/amdgpu_discovery.c
- **Access date:** 2026-07-18
- **Source type:** Official upstream source capture
- **Applicability label:** [FAMILY_APPLIES]
- **Confidence:** High for mechanism

## Decision-relevant official claims

- [OFFICIAL] Source blob SHA is `7b9bb998906d3ab9ccfb634b8c494bb6c5f9700e` at kernel commit `94515f3a7d4256a5062176b7d6ed0471938cd51a`.
- [OFFICIAL] AMDGPU discovers separate GC, UMC, NBIO, MP0, SMU and other hardware-IP versions; a gfx target is only one component of the capability tuple.

## Explicit gaps and limits

- [OPEN] Target runtime IP discovery dump.

## License / reuse

File carries the MIT-style AMD permission notice; preserve copyright and license text.

## Capture note

This is an access-dated normalized factual capture unless a raw artifact path is explicitly identified. It is not represented as a byte-for-byte server response.
