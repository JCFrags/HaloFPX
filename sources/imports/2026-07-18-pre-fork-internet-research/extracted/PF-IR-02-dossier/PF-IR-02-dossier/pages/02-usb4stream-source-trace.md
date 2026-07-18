# USB4STREAM source trace

## Patch-series lineage

V1 was posted 2026-04-28 under Message-ID `<20260428072209.3084930-1-mika.westerberg@linux.intel.com>`. V2 was posted 2026-05-11 under Message-ID `<20260511102744.1867485-1-mika.westerberg@linux.intel.com>`. Review caused the custom character-device major to be replaced with a miscdevice, removed a proposed thunderbolt-net parameter change, and added administrator documentation.

The ten integrated subjects, in series order, are:

1. `thunderbolt: Add tb_property_merge_dir()`
2. `thunderbolt: Add KUnit test for tb_property_merge_dir()`
3. `thunderbolt: Allow service drivers to specify their own properties`
4. `thunderbolt / net: Move ring_frame_size() to thunderbolt.h`
5. `thunderbolt / net: Let the service drivers configure interrupt throttling`
6. `thunderbolt: Add helper to figure size of the ring`
7. `thunderbolt: Add tb_ring_flush()`
8. `thunderbolt: Add support for ConfigFS`
9. `thunderbolt: Add support for USB4STREAM`
10. `docs: admin-guide: thunderbolt: Add instructions how to use USB4STREAM`

`[EVIDENCE-GAP]` The accessible archives exposed the message IDs, subjects, signed subsystem tag, tag target, USB pull tip and Linus merge, but not a separately verified object ID for every individual patch. No hash is inferred from ordering.

## Released source identity

`drivers/thunderbolt/stream.c` at v7.2-rc3 has Git blob `c1f5c55583d069c811d25df95f4e90136255d585`. `CONFIG_USB4_STREAM` is a tristate requiring `USB4_CONFIGFS`; the module name is `thunderbolt_stream`. ConfigFS is registered under `/sys/kernel/config/thunderbolt` by the core Thunderbolt module.

## Post-merge compatibility correction

The original source maps all TX/RX pages with `TB_MAX_FRAME_SIZE`. It later unmaps using `tb_ring_frame_size(&sf->frame)`. A short final DATA frame and the fixed 256-byte CLOSE frame therefore produce an unmap size smaller than the mapping size. Xu Rao’s one-line patch uses `TB_MAX_FRAME_SIZE`; Mika Westerberg stated it was applied to `thunderbolt.git/fixes`.

The correction is not present in the v7.2-rc3 blob. It is present in the observed fixes/linux-next lane. The individual accepted commit ID remains an explicit evidence gap; the patch text, old/new blobs and branch head are preserved.


## Sources

- **S003** — Thunderbolt v7.2 signed subsystem tag (tag aa1565cc3d6aa4bd75fce5dcf9338d763906c077; target c1bef05763c94ae284ee2881c03bf0753f8d213a)
- **S004** — Linus USB 7.2 merge commit (364f4a55c661641c02c86a849f0608d8fc3c0006; merged usb-7.2-rc1 tip 1c2b66a7d7257d2652aa41f9a860ecb96dde27dd)
- **S005** — USB4STREAM implementation source (v7.2-rc3 blob c1f5c55583d069c811d25df95f4e90136255d585)
- **S006** — USB4/USB4STREAM Kconfig (v7.2-rc3-equivalent blob 294b3227a54582536433cbb391057b99bb9df352)
- **S007** — Thunderbolt module Makefile (v7.2-rc3-equivalent blob beb054c3126b1445d26b4c14de039d23367a5100)
- **S008** — Thunderbolt ConfigFS support (v7.2-rc3-equivalent blob dc6bc363dfe8052d7f94c6e3dca3efe9fe9771be)
- **S011** — USB4STREAM v1 cover letter (Message-ID 20260428072209.3084930-1-mika.westerberg@linux.intel.com)
- **S012** — USB4STREAM v2 cover letter (Message-ID 20260511102744.1867485-1-mika.westerberg@linux.intel.com)
- **S013** — USB4STREAM DMA unmap-size fix proposal and acceptance (blob delta c1f5c55583d0..4cc86d8d6491; fixes branch head observed db79679595326fd3f6bd1e6fd0cefc3ba016039a)
