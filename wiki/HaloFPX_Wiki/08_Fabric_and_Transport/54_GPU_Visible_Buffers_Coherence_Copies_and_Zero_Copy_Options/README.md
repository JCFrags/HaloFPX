---
section_id: "54"
title: "GPU-Visible Buffers, Coherence, Copies, and Zero-Copy Options"
status: "needs-machine-validation"
last_verified: "2026-07-16"
applies_to:
  repositories: ["torvalds/linux@8cdeaa50eae8dad34885515f62559ee83e7e8dda"]
  software_versions: ["Linux 7.2-rc2 USB4STREAM", "HIP 7.2.53211 documentation", "Vulkan 1.4.356"]
  hardware_revisions: ["Radeon 8060S/gfx1151; exact project machines require validation"]
related_sections: ["19", "24", "25", "50", "53", "55", "74", "75"]
---

# 54 — GPU-Visible Buffers, Coherence, Copies, and Zero-Copy Options

Physical unified memory does not make an end-to-end path zero-copy. Every API boundary has its own ownership, mapping, synchronization, and lifetime contract.

## Current conclusion

- **[VERIFIED]** HIP can allocate mapped pinned host memory visible to the GPU, and Vulkan can expose host-visible memory, but both require explicit synchronization/coherence handling [S54-01, S54-02, S54-03].
- **[VERIFIED]** Linux dma-buf shares hardware-accessible buffers only between participating exporter/importer drivers and requires fence/cache-coherency discipline [S54-04].
- **[VERIFIED]** Linux 7.2-rc2 USB4STREAM `stream.c` copies TX from a userspace iterator into kernel pages and RX back to a userspace iterator; its file operations expose no `mmap`, splice, dma-buf import, or transport-specific io_uring command [S54-06].
- **[INFERENCE]** HIP-mapped, Vulkan host-visible, or io_uring-registered memory can reduce application staging or repeated pin/map overhead, but cannot eliminate USB4STREAM’s internal page copies without a new, reviewed kernel interface.
- **[OPEN]** The fastest correct buffer path and whether copy removal matters to real HaloFPX traffic require S54-E01–E06.

The accepted project decision keeps USB4NET/TCP/MPTCP as default and treats USB4STREAM as a reversible probe, not zero-copy [S54-L02].

See [facts](facts_and_constraints.md), [implications](design_implications.md), [procedures](procedures_and_checks.md), [questions](open_questions.md), and [sources](sources.md).
