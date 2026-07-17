---
section_id: "62"
title: "Async I/O sources"
status: "draft"
last_verified: "2026-07-17"
applies_to:
  repositories: ["fewtarius/CachyLLama@6be745998f568e379ea197fcf827baec73ff9940"]
  software_versions: []
  hardware_revisions: []
related_sections: ["21", "28"]
---

# Sources

Access date: 2026-07-17. These are source/design references; the deployment kernel, filesystem, liburing build, and feature set remain to be inventoried.

| ID | Primary source and revision | Supports | Limitations |
|---|---|---|---|
| S62-01 | [CachyLLama SSD cache source](https://github.com/fewtarius/CachyLLama/tree/6be745998f568e379ea197fcf827baec73ff9940/common), `6be74599`, 2026-07-08 | current sync I/O, tiers, readahead | not HaloKV design |
| S62-02 | [llama-ai](https://github.com/fewtarius/llama-ai/tree/1017f3dfdce3ca2b06aa9007b23295db3bb35722), `1017f3df`, 2026-07-08 | integration baseline | cache delegated to submodule |
| S62-03 | [io_uring(7)](https://man7.org/linux/man-pages/man7/io_uring.7.html), Linux man-pages 6.18, 2026 | async ring model | exact kernel support varies |
| S62-04 | Jens Axboe, [liburing tag `liburing-2.12`, dereferenced commit `e907d6a342e80b70874f93abd440b92b8a40b7bc`](https://github.com/axboe/liburing/tree/e907d6a342e80b70874f93abd440b92b8a40b7bc), accessed 2026-07-17 | fixed userspace API/examples reference for ring and registered-resource experiments | not the installed library and not proof the deployment kernel supports every operation |
| S62-05 | [Linux iomap direct I/O](https://docs.kernel.org/6.12/filesystems/iomap/operations.html), kernel 6.12 docs | page-cache bypass/direct I/O | filesystem implementation varies |
| S62-06 | Linux kernel 6.12 documentation, [cgroup v2 I/O controller](https://docs.kernel.org/6.12/admin-guide/cgroup-v2.html), accessed 2026-07-17 | writeback/I/O control | deployment kernel and configuration unresolved |

Source count is 6.
