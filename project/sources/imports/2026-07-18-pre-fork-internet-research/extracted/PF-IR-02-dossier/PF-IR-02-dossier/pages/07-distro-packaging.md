# Distro and CachyOS packaging/backport record

## CachyOS published packages

| Repository/arch | Package | Version | Build | SHA256 |
|---|---|---|---|---|
| cachyos-v3 / x86_64_v3 | `linux-cachyos-rc` | 7.2.rc3-1 | 2026-07-13 17:06 | `8d65e3132b24f28120cabcae34103949676c46fdcde20542aa1e0f427b413554` |
| cachyos-v4 / x86_64_v4 | `linux-cachyos-rc` | 7.2.rc3-1 | 2026-07-13 17:44 | `042a202c5508044e126b89268607016a39ccf6175038576e0d799228287e5774` |
| cachyos / x86_64 | `linux-cachyos` | 7.1.3-2 | 2026-07-09 | `6c63efb0b639ab91ac3a69fc0efbf1303afac16c33e557073fe0806ee6430a47` |

## Config and source provenance

At repository commit `5aeec554010d57edf03e7cad83c174fd945da8b9`, the RC config blob `69d62dfb8752dfc550e3e31011acd05bda1ae039` contains `CONFIG_USB4_STREAM=m` and `CONFIG_MPTCP=y`. The captured PKGBUILD blob is `dd4c9a88224b44c3dadb47d7c4db52aeee10cc38` and already describes `_tagrel=3`, `pkgrel=2` and source tag `cachyos-7.2-rc3-3`.

The published binary record is `7.2.rc3-1`. Therefore the moving branch snapshot is not the exact frozen build recipe for the binary. The exact package hash is strong binary identity, but source/patch provenance is incomplete until one of these closes the gap:

- the exact source package/PKGBUILD commit and config used by the build are recovered;
- the package is unpacked, `buildinfo`/module/config metadata are captured, and its source archive is matched;
- CachyOS supplies an immutable source tag or archive corresponding to the package hash.

## Backport findings

No public CachyOS packaging-tree hit for `USB4STREAM` was found outside the 7.2 RC config, and the stable 7.1.3 upstream base lacks the feature. The stable package is therefore not represented as a USB4STREAM backport. This is a source-record conclusion, not a machine inspection of every package file.

The accepted DMA-unmap correction may or may not be present in the published RC binary; the moving source pointer is insufficient to decide. Package qualification must test the exact hash rather than infer from version text.


## Sources

- **S001** — Kernel.org current release index (mainline 7.2-rc3; stable 7.1.3)
- **S025** — CachyOS linux-cachyos-rc package — x86_64_v3 (7.2.rc3-1; SHA256 8d65e3132b24f28120cabcae34103949676c46fdcde20542aa1e0f427b413554)
- **S026** — CachyOS linux-cachyos-rc package — x86_64_v4 (7.2.rc3-1; SHA256 042a202c5508044e126b89268607016a39ccf6175038576e0d799228287e5774)
- **S027** — CachyOS stable package record (linux-cachyos 7.1.3-2; x86_64 SHA256 6c63efb0b639ab91ac3a69fc0efbf1303afac16c33e557073fe0806ee6430a47)
- **S028** — CachyOS RC PKGBUILD moving snapshot (blob dd4c9a88224b44c3dadb47d7c4db52aeee10cc38; indexed repository commit 5aeec554010d57edf03e7cad83c174fd945da8b9)
- **S029** — CachyOS RC kernel config symbol evidence (config blob 69d62dfb8752dfc550e3e31011acd05bda1ae039 at repository commit 5aeec554010d57edf03e7cad83c174fd945da8b9)
