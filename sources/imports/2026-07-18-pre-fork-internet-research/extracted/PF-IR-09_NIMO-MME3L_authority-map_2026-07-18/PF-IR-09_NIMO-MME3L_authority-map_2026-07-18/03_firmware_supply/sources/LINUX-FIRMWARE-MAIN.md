# linux-firmware main branch snapshot

- **Source ID:** `LINUX-FIRMWARE-MAIN`
- **Authority:** linux-firmware upstream
- **Canonical URL:** https://kernel.googlesource.com/pub/scm/linux/kernel/git/firmware/linux-firmware/+/refs/heads/main
- **Access date:** 2026-07-18
- **Source type:** Official upstream repository
- **Applicability label:** [FAMILY_APPLIES]
- **Confidence:** High for repository state; Medium for loaded target firmware

## Decision-relevant official claims

- [OFFICIAL] Captured main branch points to commit `924d73c9a2501a256d18a26cbe640548c70b3a9a`, dated 2026-07-16.
- [OFFICIAL] The repository contains AMDGPU blobs including `gc_11_5_1_*` firmware names consistent with gfx1151-era IP.
- [OFFICIAL] linux-firmware provenance is repository/commit/file based; runtime loading must be measured separately.

## Explicit gaps and limits

- [OPEN] Exact distro package commit, per-file hashes, and loaded versions on the target.
- No claim that every `gc_11_5_1_*` file is loaded by PCI 1002:1586 rev c1.

## License / reuse

linux-firmware blobs are redistributable under per-vendor license files and WHENCE entries; source tree metadata must accompany redistribution.

## Capture note

This is an access-dated normalized factual capture unless a raw artifact path is explicitly identified. It is not represented as a byte-for-byte server response.
