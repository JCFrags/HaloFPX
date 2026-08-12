# Executive decision record

## Outcome

The externally supportable roster is not “Linux 7.2” as one undifferentiated target. It separates the current stable release, the current mainline prepatch, the accepted fixes lane, linux-next, and packaged CachyOS binaries.

**ABI target:** `CONFIG_USB4_STREAM` + Thunderbolt ConfigFS + `/dev/tbstreamX`. This is a byte-stream character-device interface backed by kernel-owned DMA pages and userspace copies. It is not an ioctl, mmap, registered-buffer, dma-buf or zero-copy ABI.

**TCP/MPTCP boundary:** ordinary TCP fallback can occur during initial MP_CAPABLE negotiation and on passive acceptance of a non-MPTCP peer. Loss of one additional subflow is a path failure, not an automatic conversion of an established MPTCP connection to TCP. Established fallback/reset is state- and protocol-error-specific.

**Qualification boundary:** K3, a source-frozen v7.2-rc3 plus the accepted one-line stream DMA-unmap correction, is sufficiently evidenced to enter reversible machine qualification. K5, the exact-hash CachyOS binary, can enter package-content qualification, but a supportable source-provenance claim remains blocked until the binary’s frozen recipe/config/source tag is recovered or independently reconstructed.

No installation, boot, cable test, throughput test, fault injection or rollback experiment was performed. No claim is labeled `[MACHINE-TESTED]`.

## Candidate roster

| ID | Candidate | Status | USB4STREAM | Qualification posture |
|---|---|---|---|---|
| K1 | Linux v7.1.3 stable | `[RELEASED]` | Absent | Control and rollback lane |
| K2 | Linux v7.2-rc3 | `[RELEASED]` | Present; known unmap-size defect remains | Source comparison only unless patched |
| K3 | v7.2-rc3 + accepted unmap correction | `[BACKPORTED]` | Present with minimal known correction | Enter reversible machine qualification |
| K4 | linux-next next-20260717 | `[RELEASED]` integration snapshot | Present; correction integrated | Integration reference; avoid conflating with minimal patch lane |
| K5 | CachyOS 7.2.rc3-1 v3/v4 | `[PACKAGED]` | Package base is 7.2-rc3; captured moving config enables module | Hash-pinned package-content qualification; provenance gap remains |
| K6 | CachyOS 7.1.3-2 | `[PACKAGED]` | No upstream feature; no public backport found | Control/rollback lane |

## Normative-versus-implemented boundary

RFC 8684 and erratum 6609 are publicly normative for MPTCP. USB4 v2.0 November 2025 and Type-C 2.5 are publicly downloadable under limited USB-IF terms but are not redistributed here. PCIe 7.0 normative text is member controlled. Linux source is authoritative for released Linux behavior; it is not proof of cross-vendor conformance, physical-layer goodput, GPU-direct, or proprietary connection-manager behavior.


## Sources

- **S001** — Kernel.org current release index (mainline 7.2-rc3; stable 7.1.3)
- **S002** — Linux v7.2-rc3 tag metadata (tag object 1137d8b5df06137fb49513cc923b3b24d94cb809; commit a13c140cc289c0b7b3770bce5b3ad42ab35074aa)
- **S005** — USB4STREAM implementation source (v7.2-rc3 blob c1f5c55583d069c811d25df95f4e90136255d585)
- **S013** — USB4STREAM DMA unmap-size fix proposal and acceptance (blob delta c1f5c55583d0..4cc86d8d6491; fixes branch head observed db79679595326fd3f6bd1e6fd0cefc3ba016039a)
- **S015** — MPTCP sysctl documentation (v7.2-rc3 blob b9b5f58e…)
- **S019** — RFC 8684 — TCP Extensions for Multipath Operation (RFC 8684; obsoletes RFC 6824)
- **S021** — USB4 Specification v2.0 public download metadata (USB4 Specification November 2025 package; page updated 2026-04-02)
- **S024** — PCI Express Base Specification public overview/access record (PCI Express Base Specification Revision 7.0)
- **S025** — CachyOS linux-cachyos-rc package — x86_64_v3 (7.2.rc3-1; SHA256 8d65e3132b24f28120cabcae34103949676c46fdcde20542aa1e0f427b413554)
- **S026** — CachyOS linux-cachyos-rc package — x86_64_v4 (7.2.rc3-1; SHA256 042a202c5508044e126b89268607016a39ccf6175038576e0d799228287e5774)
- **S028** — CachyOS RC PKGBUILD moving snapshot (blob dd4c9a88224b44c3dadb47d7c4db52aeee10cc38; indexed repository commit 5aeec554010d57edf03e7cad83c174fd945da8b9)
- **S029** — CachyOS RC kernel config symbol evidence (config blob 69d62dfb8752dfc550e3e31011acd05bda1ae039 at repository commit 5aeec554010d57edf03e7cad83c174fd945da8b9)
