---
section_id: "62"
title: "Async I/O facts and constraints"
status: "needs-machine-validation"
last_verified: "2026-07-16"
applies_to:
  repositories: ["fewtarius/CachyLLama@6be745998f568e379ea197fcf827baec73ff9940"]
  software_versions: []
  hardware_revisions: []
related_sections: ["21", "28"]
---

# Facts and constraints

**[VERIFIED]** CachyLLama's cache reads/writes exact byte counts in 64 MiB chunks. Its cold promotion performs `posix_fadvise(...WILLNEED)` then synchronous read; hot and warm tiers are in-process byte vectors with configurable budgets [S62-01].

**[VERIFIED]** io_uring setup/enter/register APIs separate submission and completion; registered buffers/files can reduce per-I/O setup but pin resources and require lifecycle discipline [S62-03][S62-04].

**[VERIFIED]** Linux direct I/O bypasses the page cache and has filesystem/device alignment constraints [S62-05]. It is not inherently faster than buffered I/O.

**[INFERENCE]** Tokenization can expose a candidate prefix early enough to begin metadata lookup and read-ahead before model evaluation. False-positive prefetch must be bounded to avoid displacing useful cache pages.

**[INFERENCE]** Strix Halo unified memory does not make SSD pages automatically GPU-ready. Restored bytes still need a validated representation, alignment, ownership, and synchronization path before backend use.

