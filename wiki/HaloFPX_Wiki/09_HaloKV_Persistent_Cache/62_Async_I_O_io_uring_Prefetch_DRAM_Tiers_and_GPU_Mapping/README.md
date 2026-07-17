---
section_id: "62"
title: "Async I/O, io_uring, Prefetch, DRAM Tiers, and GPU Mapping"
status: "needs-machine-validation"
last_verified: "2026-07-17"
applies_to:
  repositories: ["fewtarius/CachyLLama@6be745998f568e379ea197fcf827baec73ff9940"]
  software_versions: ["Linux io_uring; exact kernel unresolved"]
  hardware_revisions: ["dual Strix Halo"]
related_sections: ["19", "21", "28", "54", "59", "63"]
---

# 62 - Async cache I/O design

- **[VERIFIED]** Pinned CachyLLama uses synchronous chunked `pread`/`pwrite`, hot/warm vectors, and Linux `POSIX_FADV_WILLNEED`; no io_uring path was identified [S62-01].
- **[VERIFIED]** io_uring provides shared submission/completion queues and supports asynchronous I/O [S62-03].
- **[RECOMMENDATION]** Start with buffered asynchronous reads/writes and measured page-cache behavior. Add direct I/O only if it wins under matched load.
- **[OPEN]** Safe, useful GPU-visible mapping on the selected Strix Halo kernel/ROCm stack is unproven.

## Research split

- **Internet/source-code research completed:** pinned CachyLLama I/O/tiering paths plus fixed Linux 6.12 documentation, Linux man-pages 6.18, and liburing 2.12 references establish candidate mechanisms, not performance.
- **Target-machine work required:** identify the deployed kernel/filesystem/liburing/ROCm pins and compare buffered, io_uring, direct-I/O, prefetch, staging, cancellation, and pressure behavior under matched two-host loads.
- **Contingent decisions:** queue depths, registered-buffer budgets, direct-I/O use, prefetch trigger, tier sizes, and GPU-visible mapping remain unapproved pending those measurements.
