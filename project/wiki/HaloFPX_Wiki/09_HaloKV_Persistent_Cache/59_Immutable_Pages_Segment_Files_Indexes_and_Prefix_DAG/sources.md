---
section_id: "59"
title: "HaloKV format primary sources"
status: "draft"
last_verified: "2026-07-17"
applies_to:
  repositories: ["HaloKV design", "fewtarius/CachyLLama"]
  software_versions: ["CachyLLama 6be745998f568e379ea197fcf827baec73ff9940", "format proposal v0"]
  hardware_revisions: ["dual Strix Halo NVMe hosts; exact filesystems pending"]
related_sections: ["57", "58", "60", "61", "62", "63", "64", "65"]
---

# HaloKV format primary sources

Accessed 2026-07-16.

| ID | Source/revision | Supports | Limitations |
|---|---|---|---|
| S59-01 | CachyLLama [`kv-ssd-cache.h/.cpp`](https://github.com/fewtarius/CachyLLama/tree/6be745998f568e379ea197fcf827baec73ff9940/common), commit `6be74599` | Predecessor v3 record/index/file behavior | Not crash-safe HaloKV proof. |
| S59-02 | SQLite, [Database File Format](https://www.sqlite.org/fileformat.html), accessed 2026-07-16 | Main/journal/WAL state, pages and commit frames | SQLite format is not copied into HaloKV. |
| S59-03 | SQLite, [WAL-mode File Format](https://www.sqlite.org/walformat.html), accessed 2026-07-16 | WAL, wal-index and recovery model | VFS/filesystem-dependent details remain. |
| S59-04 | The Open Group, [`rename()` POSIX.1-2024](https://pubs.opengroup.org/onlinepubs/9799919799/functions/rename.html) | Directory-entry rename semantics | Does not alone guarantee application durability. |
| S59-05 | The Open Group, [`fsync()`](https://pubs.opengroup.org/onlinepubs/9799919799/functions/fsync.html), POSIX.1-2024 | Synchronized I/O completion interface | Device/filesystem failure semantics need testing. |
| S59-06 | Linux kernel 6.12 documentation, [iomap supported file operations](https://docs.kernel.org/6.12/filesystems/iomap/operations.html), accessed 2026-07-17 | Buffered/page-cache and direct-I/O behavior/alignment | Documentation pin is not the selected deployment kernel; re-audit its exact source/configuration. |
| S59-07 | SQLite, [Atomic Commit](https://www.sqlite.org/atomiccommit.html), accessed 2026-07-16 | Failure-aware atomic publication principles | Rollback-mode exposition; HaloKV design differs. |
| S59-08 | BLAKE3 team, [BLAKE3 specification at commit `ea51a3ac997288bf690ee82ac9cfc8b3e0e60f2a`](https://github.com/BLAKE3-team/BLAKE3-specs/blob/ea51a3ac997288bf690ee82ac9cfc8b3e0e60f2a/blake3.pdf), accessed 2026-07-17 | Alternative digest design context only | HaloKV v1 currently recommends SHA-256 in Section 57; this source does not authorize an algorithm change. |

## Status

The segment/page/DAG schemas are **[RECOMMENDATION]** proposals, not an implemented or verified format. Primary sources support mechanisms and predecessor facts only.
