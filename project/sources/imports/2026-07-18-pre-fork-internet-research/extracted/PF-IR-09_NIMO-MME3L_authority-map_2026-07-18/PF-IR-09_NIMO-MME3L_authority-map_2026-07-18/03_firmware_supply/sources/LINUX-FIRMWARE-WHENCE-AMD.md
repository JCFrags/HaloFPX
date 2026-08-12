# linux-firmware WHENCE and AMDGPU license/provenance

- **Source ID:** `LINUX-FIRMWARE-WHENCE-AMD`
- **Authority:** linux-firmware upstream
- **Canonical URL:** https://kernel.googlesource.com/pub/scm/linux/kernel/git/firmware/linux-firmware/+/refs/heads/main/WHENCE
- **Access date:** 2026-07-18
- **Source type:** Official upstream repository metadata
- **Applicability label:** [FAMILY_APPLIES]
- **Confidence:** Medium

## Decision-relevant official claims

- [OFFICIAL] WHENCE records AMDGPU firmware families including SDMA 6.1.x and SMU 14.0.2/14.0.3 entries and points to `LICENSE.amdgpu`.
- [OFFICIAL] Repository inclusion and Signed-off-by provenance support distribution provenance, not device-side signature or anti-rollback proof.

## Explicit gaps and limits

- [OPEN] Actual MP0/SMU/SDMA/GFX IP discovery tuple and loaded filenames.
- [SIGNATURE_UNPROVEN] Runtime cryptographic verification path.

## License / reuse

Redistribution governed by `LICENSE.amdgpu` and WHENCE. Preserve license and provenance files with blobs.

## Capture note

This is an access-dated normalized factual capture unless a raw artifact path is explicitly identified. It is not represented as a byte-for-byte server response.
