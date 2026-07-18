# amdgpu_ras source at commit 94515f3a

- **Source ID:** `KERNEL-SOURCE-AMDGPU-RAS`
- **Authority:** Linux kernel upstream source
- **Canonical URL:** https://github.com/torvalds/linux/blob/94515f3a7d4256a5062176b7d6ed0471938cd51a/drivers/gpu/drm/amd/amdgpu/amdgpu_ras.c
- **Access date:** 2026-07-18
- **Source type:** Official upstream source capture
- **Applicability label:** [UNKNOWN]
- **Confidence:** Medium

## Decision-relevant official claims

- [OFFICIAL] Source blob SHA is `764cd49504083c077c7952b8408e293eebbaf190` at kernel commit `94515f3a7d4256a5062176b7d6ed0471938cd51a`.
- [OFFICIAL] RAS enablement is gated by ASIC/IP discovery support, PSP or VBIOS capability, module policy, and block mask.
- [OFFICIAL] The captured IP-discovery whitelist includes MP0 14.0.3 but not MP0 14.0.2.
- [OFFICIAL] CE/UE counts exist only after supported blocks initialize.

## Explicit gaps and limits

- [OPEN] Actual target MP0, UMC, NBIO, DF, PSP, and VBIOS capability tuple.
- If target MP0 is 14.0.2, this source revision fails the shown IP-discovery whitelist; if 14.0.3, later capability gates still apply.
- gfx1151 alone is insufficient to classify support.

## License / reuse

File carries the MIT-style AMD permission notice; preserve copyright and license text.

## Capture note

This is an access-dated normalized factual capture unless a raw artifact path is explicitly identified. It is not represented as a byte-for-byte server response.
