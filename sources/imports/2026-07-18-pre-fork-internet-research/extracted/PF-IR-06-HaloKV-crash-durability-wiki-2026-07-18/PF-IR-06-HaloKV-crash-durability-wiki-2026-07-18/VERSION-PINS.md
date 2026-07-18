# Version and revision pins

Access date: **2026-07-18** (America/Los_Angeles).

| Component | Semantic pin | Exact revision / artifact | Role |
|---|---|---|---|
| Linux stable kernel | **7.1.3**, released 2026-07-04 | commit `199c9959d3a9b53f346c221757fc7ac507fbac50`; annotated tag object `113be0295f4f632d61f4e9c93322f70e2365f54d` | Kernel/VFS/filesystem/io_uring semantic anchor |
| Linux mainline | 7.2-rc3 at access | not used as contract anchor | Recency note only |
| liburing | **2.15**, released 2026-06-29 | commit `d41bf9220ec39277ff235379e9089d9e0fd6c2a5` | Userspace API/manual anchor |
| Linux man-pages | **6.18**, released 2026-04-22 | tag `man-pages-6.18`; tarball SHA-256 `c934fadc8b59748c68227a34f6581d2ddf8282b73cdcd52546c8cd88b74b24d1` | Syscall semantics and errors |
| ext4 docs | Linux 7.1.3 tree | `Documentation/admin-guide/ext4.rst`; blob `ac0c709ea9e7c1f15475b7f0b7b550968f3fdbc0` | ext4 profile |
| XFS docs | Linux 7.1.3 tree | `Documentation/admin-guide/xfs.rst`; blob `acdd4b65964c058647a7b383d8c2c778777a538e` | XFS profile |
| xfsprogs release | **7.1.0**, listed 2026-07-13 | kernel.org release-index receipt; kernel docs above remain semantic pin | Tooling recency record |
| Btrfs kernel docs | Linux 7.1.3 tree | `Documentation/filesystems/btrfs.rst`; blob `a81db8f54d689300a49ff78cb06ba783dcdad21d` | Kernel overview |
| btrfs-progs docs | **7.1**, released 2026-07-14 | commit `4ab0e80be9e3bb1db2e6038e6d4316d35fb7ba8b`; mount-option blob `8f65b8311e7eee1043e690ec40ff73fca395f686` | Current admin/mount semantics |
| POSIX | POSIX.1-2024 | The Open Group issue `9799919799` | Portable namespace/open floor |

## Pinning policy

- Kernel and liburing claims cite immutable commits and upstream blob IDs where available.
- Web/standards pages are stored as **normalized receipts with short excerpts**, not falsely described as byte-for-byte captures.
- Raw-capture scope is literal in `matrices/source-register.csv`: `normalized source excerpt`, `official web receipt`, or `metadata receipt`.
- The deployed kernel, filesystem feature set, mount options, block stack, firmware and device remain unpinned until `T001` records them.
