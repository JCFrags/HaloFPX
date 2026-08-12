# OEM authority — NIMO / Nimo Direct MME3L

## Exact identity state

- `[MEASURED]` Nimo Direct `MME3L`; `NIMO Mini PC board v1.0`; BIOS `3.05`, date `2025-10-11`.
- `[FAMILY_APPLIES]` NIMO publicly markets and supports an “AI 395 Minipc” based on Ryzen AI MAX+ 395 / Strix Halo.
- `[OPEN]` The public product/support pages do not bind the measured `MME3L` and board-v1.0 strings to a firmware support branch.

## Public update authority

The captured support article publishes Windows OS images and a manual. It does not publish an exact motherboard BIOS, AMD PI/AGESA manifest, release notes, capsule hash, signature chain, lowest permitted version, downgrade policy, crisis-recovery procedure, or service bulletin.

**Classification:** `[NO_PUBLIC_PACKAGE] [SIGNATURE_UNPROVEN] [ROLLBACK_UNPROVEN] [HOLD]`.

## Required OEM evidence before rollout

1. Exact model, board revision, EC/controller revision and region applicability.
2. Signed BIOS/capsule package hash and signer/certificate verification path.
3. AMD `StrixHaloPI-FP11` version and incorporated security bulletin list.
4. Embedded PSP/SMU/microcode/VBIOS/USB4 component version inventory.
5. NVRAM/settings-reset behavior, Secure Boot key behavior, power-loss behavior, and recovery path.
6. Downgrade/rollback policy and a tested recovery method on equivalent hardware.

The measured BIOS date predates AMD's 2025-11-25 and 2025-12-29 release-to-OEM dates for Strix Halo PI 1.0.0.2a/1.0.0.2b. This is an `[INFERRED]` update gap, not proof of the internal PI version.
