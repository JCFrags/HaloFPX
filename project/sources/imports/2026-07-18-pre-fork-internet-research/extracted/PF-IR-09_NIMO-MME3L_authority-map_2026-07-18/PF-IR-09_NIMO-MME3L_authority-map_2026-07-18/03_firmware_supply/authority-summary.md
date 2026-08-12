# Firmware supply-chain authorities — LVFS/fwupd and linux-firmware

## LVFS/fwupd

LVFS provides repository metadata, matching, checksums, and detached signatures for firmware distribution. fwupd verifies metadata/archive trust under policy. These controls answer whether an update came from a trusted repository; they do not, by themselves, prove that the target device authenticates the vendor payload or enforces anti-rollback.

The preserved public `PULP_MANIFEST` contains no literal filename match for the measured OEM, SSD, firmware, or PCI strings. This is `[OPEN]`, because actual update applicability is GUID/instance-ID and metadata based, and private/embargo firmware is excluded from the public manifest.

Required local proof: `fwupdmgr get-devices --json`, device GUIDs/instance IDs, flags, current and lowest version, bootloader version, remote, update metadata, CAB/Jcat hashes, and update history.

## linux-firmware

The captured upstream branch is commit `924d73c9a2501a256d18a26cbe640548c70b3a9a` dated 2026-07-16. The tree contains `gc_11_5_1_*` firmware and WHENCE metadata for related AMDGPU IP families. Repository inclusion is not evidence that the target loaded each file.

Required local proof: distribution package/source commit, initramfs contents, per-file hashes, kernel boot log, and AMDGPU firmware/IP discovery output. Kernel and linux-firmware should be treated as one tested rollback unit.

## Signature and rollback classification

| Layer | What can be proven | What remains unproven |
|---|---|---|
| LVFS/Jcat | Repository origin, detached signature/checksum | Device vendor signature, authorization, hardware anti-rollback |
| fwupd plugin | GUID matching, client version policy, update result | Board-level recovery and physical rollback |
| linux-firmware repository | Blob provenance and redistribution license | Runtime device authentication and exact loaded tuple |
