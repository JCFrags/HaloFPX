# Kernel and package candidate roster

## Exact pins

| Candidate | Pin | Feature/fix state | Evidence class |
|---|---|---|---|
| Linux stable | `v7.1.3`, released 2026-07-04 | USB4STREAM absent | `[RELEASED]` |
| Linux mainline prepatch | tag object `1137d8b5df06137fb49513cc923b3b24d94cb809`, commit `a13c140cc289c0b7b3770bce5b3ad42ab35074aa` | USB4STREAM present; unmap-size defect present | `[RELEASED]` |
| Thunderbolt merge source | signed tag `aa1565cc3d6aa4bd75fce5dcf9338d763906c077`, target `c1bef05763c94ae284ee2881c03bf0753f8d213a` | Feature series integrated | `[RELEASED]` |
| Linus USB merge | `364f4a55c661641c02c86a849f0608d8fc3c0006`, pulled tip `1c2b66a7d7257d2652aa41f9a860ecb96dde27dd` | Feature enters mainline | `[RELEASED]` |
| Minimal correction | blob delta `c1f5c55583d0..4cc86d8d6491`; fixes head observed `db79679595326fd3f6bd1e6fd0cefc3ba016039a` | use `TB_MAX_FRAME_SIZE` for DMA unmap | `[BACKPORTED]` + individual-commit `[EVIDENCE-GAP]` |
| linux-next | `next-20260717` | correction integrated with unrelated churn | `[RELEASED]` integration snapshot |
| CachyOS v3 package | `linux-cachyos-rc-7.2.rc3-1-x86_64_v3.pkg.tar.zst`, SHA256 `8d65e3132b24f28120cabcae34103949676c46fdcde20542aa1e0f427b413554` | package record pinned | `[PACKAGED]` |
| CachyOS v4 package | `linux-cachyos-rc-7.2.rc3-1-x86_64_v4.pkg.tar.zst`, SHA256 `042a202c5508044e126b89268607016a39ccf6175038576e0d799228287e5774` | package record pinned | `[PACKAGED]` |

## Why K3 is the source qualification lane

K2 contains the original stream source blob and the mismatched DMA unmap size. K3 changes exactly one expression in `tbstream_ring_free()`. This gives a narrow, auditable and reversible delta instead of importing linux-next. The dossier does not assert that K3 is operationally safe; it asserts that the provenance is sufficiently bounded to justify local compatibility, suspend/resume, detach/reconnect, IOMMU warning and rollback experiments.

## Why the CachyOS binary remains separate

The package hash and publication metadata are exact. The package page’s source link resolves to a moving packaging branch whose captured PKGBUILD is already at a later `pkgrel`/source tag. The captured config at repository commit `5aeec554010d57edf03e7cad83c174fd945da8b9` sets `CONFIG_USB4_STREAM=m` and `CONFIG_MPTCP=y`, but it is not proven to be the exact config used for the `7.2.rc3-1` binaries. Package-content inspection can close the module/config side; source-archive and recipe recovery must close provenance.


## Sources

- **S001** — Kernel.org current release index (mainline 7.2-rc3; stable 7.1.3)
- **S002** — Linux v7.2-rc3 tag metadata (tag object 1137d8b5df06137fb49513cc923b3b24d94cb809; commit a13c140cc289c0b7b3770bce5b3ad42ab35074aa)
- **S003** — Thunderbolt v7.2 signed subsystem tag (tag aa1565cc3d6aa4bd75fce5dcf9338d763906c077; target c1bef05763c94ae284ee2881c03bf0753f8d213a)
- **S004** — Linus USB 7.2 merge commit (364f4a55c661641c02c86a849f0608d8fc3c0006; merged usb-7.2-rc1 tip 1c2b66a7d7257d2652aa41f9a860ecb96dde27dd)
- **S005** — USB4STREAM implementation source (v7.2-rc3 blob c1f5c55583d069c811d25df95f4e90136255d585)
- **S013** — USB4STREAM DMA unmap-size fix proposal and acceptance (blob delta c1f5c55583d0..4cc86d8d6491; fixes branch head observed db79679595326fd3f6bd1e6fd0cefc3ba016039a)
- **S025** — CachyOS linux-cachyos-rc package — x86_64_v3 (7.2.rc3-1; SHA256 8d65e3132b24f28120cabcae34103949676c46fdcde20542aa1e0f427b413554)
- **S026** — CachyOS linux-cachyos-rc package — x86_64_v4 (7.2.rc3-1; SHA256 042a202c5508044e126b89268607016a39ccf6175038576e0d799228287e5774)
- **S027** — CachyOS stable package record (linux-cachyos 7.1.3-2; x86_64 SHA256 6c63efb0b639ab91ac3a69fc0efbf1303afac16c33e557073fe0806ee6430a47)
- **S028** — CachyOS RC PKGBUILD moving snapshot (blob dd4c9a88224b44c3dadb47d7c4db52aeee10cc38; indexed repository commit 5aeec554010d57edf03e7cad83c174fd945da8b9)
- **S029** — CachyOS RC kernel config symbol evidence (config blob 69d62dfb8752dfc550e3e31011acd05bda1ae039 at repository commit 5aeec554010d57edf03e7cad83c174fd945da8b9)
- **S030** — linux-next integration record (next-20260717)
