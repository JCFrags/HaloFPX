---
section_id: "54"
title: "Buffer-path open questions"
status: "needs-machine-validation"
last_verified: "2026-07-16"
applies_to:
  repositories: ["torvalds/linux@8cdeaa50eae8dad34885515f62559ee83e7e8dda"]
  software_versions: ["Linux 7.2-rc2 USB4STREAM", "HIP", "Vulkan", "io_uring"]
  hardware_revisions: ["Radeon 8060S/gfx1151"]
related_sections: ["19", "24", "25", "50", "55"]
---

# Open questions

| ID | Question | Resolution |
|---|---|---|
| S54-OQ01 | Which HIP mapped/coherence capabilities work correctly on both exact nodes? | S54-E01/E03 |
| S54-OQ02 | Which Vulkan host-visible types and external-memory features are exposed by the chosen stack? | S54-E01/E04 |
| S54-OQ03 | Which path minimizes GPU-produced-to-peer-GPU latency for real shapes? | S54-E06 |
| S54-OQ04 | How many copies occur, where, and what cycles/bandwidth do they consume? | Approved tracing plus E02–E06 |
| S54-OQ05 | Do io_uring fixed buffers reduce useful overhead for TCP or USB4STREAM without changing copy count? | S54-E05 |
| S54-OQ06 | What pinned/registered pool size is safe under model, KV, NVMe, and dual-rail contention? | S54-E06 plus Section 19 |
| S54-OQ07 | Can amdgpu/RADV export a suitable dma-buf and could a future USB4 importer honor its fences safely? | Source audit and separate proposal |
| S54-OQ08 | Are noncoherent bulk buffers faster after required flush/barrier cost? | S54-E03/E04 |
| S54-OQ09 | Does copy removal have enough real-workload value to justify a kernel ABI? | Section 55 decision gate |

**[OPEN]** No zero-copy claim is admitted until the actual buffer lifecycle and traced data movement prove it.
