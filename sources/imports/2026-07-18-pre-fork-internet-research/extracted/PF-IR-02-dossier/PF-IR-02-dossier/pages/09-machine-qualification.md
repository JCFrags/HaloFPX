# Reversible machine-qualification plan

This is a test plan, not a record of completed tests.

## Pre-install provenance gate

1. Freeze the exact kernel source commit/tag, config, compiler, patch series and package/archive hashes.
2. For K3, apply only the preserved one-line unmap patch and record `git diff`, resulting stream blob and build ID.
3. For K5, verify the package SHA256, unpack without installing, capture `.BUILDINFO`, `.PKGINFO`, `usr/lib/modules/*`, `modules.alias`, `modules.dep`, `config`, `vmlinuz` build ID and `modinfo thunderbolt_stream` from the unpacked tree.
4. Confirm `CONFIG_USB4`, `CONFIG_USB4_CONFIGFS`, `CONFIG_CONFIGFS_FS`, `CONFIG_USB4_STREAM`, `CONFIG_MPTCP` and required network options.

## Reversible boot/rollback gate

- Install into a separate boot entry; retain the known-good kernel and initramfs.
- Verify console/remote recovery and a one-command default-entry rollback.
- Record Secure Boot/module-signing state, IOMMU/DMA-remapping state, Thunderbolt security level and firmware/controller/cable identities.

## USB4STREAM ABI tests

- Create/remove ConfigFS groups before and after peer appearance.
- Exercise automatic, explicit, unavailable and zero HopIDs; invalid ring/throttling values; writes while opened.
- Test blocking and nonblocking open/read/write, signal interruption, positive short writes, partial reads, multiple readers/writers and close-on-last-user.
- Capture `poll`, `select`, level-triggered and edge-triggered epoll behavior for DATA, peer CLOSE, ConfigFS removal, cable removal and reconnect.
- Fault-inject CRC/overrun where hardware/tools permit; correlate kernel log, read wakeup and returned bytes/errors.
- Suspend/resume and cable detach/re-attach with open descriptors; verify data ordering/loss explicitly.
- Monitor IOMMU/DMA API warnings during close and repeated open/close; this is the direct discriminator for the unmap-size correction.

## thunderbolt-net tests

- Capture negotiated capability flags, MTU bounds, ethtool data, ring/NAPI behavior and all netdev counters.
- Normalize RX frame counters versus completed Ethernet packets before interpreting rates.
- Exercise login retry, peer logout, cable removal, netdev down/up, suspend/resume, MTU extremes, fragmented/GSO traffic, checksum paths and memory-pressure replenish failures.
- Test Linux↔Linux and, where required, Linux↔Windows USB4NET separately.

## MPTCP tests

- Record all `net.mptcp.*` sysctls, PM mode, endpoints, limits and scheduler.
- Use packet capture to distinguish MP_CAPABLE attempts, plain-TCP fallback, MP_JOIN success/failure, ADD_ADDR and DSS mappings.
- Test active SYN fallback at configured retransmission counts and passive plain-TCP acceptance.
- Remove one additional subflow and verify the MPTCP connection remains MPTCP; separately remove the last subflow and measure `close_timeout` behavior.
- Induce no-progress intervals to verify `stale_loss_cnt` strict-threshold behavior and backup-path selection.

## Acceptance record

A candidate may receive `[MACHINE-TESTED]` only after the exact machine, controller firmware, cable identity, peer OS/kernel, source/package hash, test script hash and raw logs are attached to a new evidence capture. This dossier intentionally contains no such label.


## Sources

- **S005** — USB4STREAM implementation source (v7.2-rc3 blob c1f5c55583d069c811d25df95f4e90136255d585)
- **S013** — USB4STREAM DMA unmap-size fix proposal and acceptance (blob delta c1f5c55583d0..4cc86d8d6491; fixes branch head observed db79679595326fd3f6bd1e6fd0cefc3ba016039a)
- **S014** — thunderbolt-net implementation source audit capture (v7.2-rc3 blob 02a91650561ab1556d28c4945cae7f62a704b1f3)
- **S015** — MPTCP sysctl documentation (v7.2-rc3 blob b9b5f58e…)
- **S016** — MPTCP control defaults and close handling (v7.2-rc3 net/mptcp/ctrl.c and protocol.c)
- **S017** — MPTCP scheduler and stale-subflow source audit (v7.2-rc3 net/mptcp/sched.c and protocol.c)
- **S025** — CachyOS linux-cachyos-rc package — x86_64_v3 (7.2.rc3-1; SHA256 8d65e3132b24f28120cabcae34103949676c46fdcde20542aa1e0f427b413554)
- **S026** — CachyOS linux-cachyos-rc package — x86_64_v4 (7.2.rc3-1; SHA256 042a202c5508044e126b89268607016a39ccf6175038576e0d799228287e5774)
- **S029** — CachyOS RC kernel config symbol evidence (config blob 69d62dfb8752dfc550e3e31011acd05bda1ae039 at repository commit 5aeec554010d57edf03e7cad83c174fd945da8b9)
