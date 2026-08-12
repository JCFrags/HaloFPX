---
section_id: "17"
title: "Strix Halo source register"
status: "draft"
last_verified: "2026-07-16"
applies_to:
  repositories: ["llvm/llvm-project", "torvalds/linux", "mesa/mesa"]
  software_versions: ["ROCm 7.14.0", "ROCm 7.1.1"]
  hardware_revisions: ["AMD Ryzen AI Max+ 395 / Radeon 8060S"]
related_sections: ["02", "18", "19", "23", "25", "37"]
---

# Sources

All records were accessed 2026-07-16. These are primary vendor, project documentation, or exact upstream source snapshots. No third-party benchmark is used.

## S17-SRC-001

- **Title/publisher:** AMD Ryzen AI Max+ 395 product specifications — AMD
- **URL:** https://www.amd.com/en/products/processors/laptop/ryzen/ai-300-series/amd-ryzen-ai-max-plus-395.html
- **Revision/date:** live product record; no document revision exposed; accessed 2026-07-16
- **Supports:** codename, CPU cores/threads/clocks/caches/extensions, die count/socket, memory interface/capacity/speed, GPU count/frequency, I/O, power/temperature, codecs, NPU TOPS.
- **Limitations:** mutable page; clocks/TOPS are maxima; “graphics core” is used as AMD's listed unit and correlated with CU count only through ROCm's table; no CCD/CCX, interconnect, latency, or achieved-bandwidth specification.

## S17-SRC-002

- **Title/publisher:** ROCm 7.14.0 compatibility matrix — AMD ROCm documentation
- **URL:** https://rocm.docs.amd.com/en/latest/compatibility/compatibility-matrix.html
- **Revision/date:** ROCm 7.14.0; page dated 2026-07-16; accessed 2026-07-16
- **Supports:** Ryzen AI Max+ 395 and Radeon 8060S family mapping to `gfx1151`; listed OS/driver support context.
- **Limitations:** mutable `latest` rendering; umbrella listing does not prove every ROCm component, framework, kernel, or non-listed distribution works. Preserve a snapshot when the stack is frozen.

## S17-SRC-003

- **Title/publisher:** AMD GPU specifications — AMD ROCm documentation
- **URL:** https://rocm.docs.amd.com/en/latest/reference/gpu-specs.html
- **Revision/date:** ROCm 7.14.0 rendering; accessed 2026-07-16
- **Supports:** RDNA 3.5, `gfx1151`, 40 CUs, wave sizes, LDS, cache hierarchy, register-file sizes and GFXIP for Radeon 8060S.
- **Limitations:** row names Ryzen AI Max+ **PRO** 395; consumer applicability is based on the same Radeon 8060S identity and requires S17-EXP-002 confirmation; table values are architectural, not achieved application capacity/performance.

## S17-SRC-004

- **Title/repository:** User Guide for AMDGPU Backend — LLVM project
- **URL:** https://github.com/llvm/llvm-project/blob/c571b0bd7330a4b737ad7dec31e7f2b52edd3953/llvm/docs/AMDGPUUsage.rst
- **Revision/date:** commit `c571b0bd7330a4b737ad7dec31e7f2b52edd3953`, main observed 2026-07-16; accessed 2026-07-16
- **Supports:** `gfx1151`/`amdgpu11.51`, APU classification, Radeon 8060S example product, wave64/cumode features, flat scratch/packed IDs, VGPR limitation.
- **Limitations:** compiler documentation at a moving development commit; runtime support must be checked against the installed ROCm release. A previously crawled mutable rendering showed shifted product rows, which is why this section cites the exact source commit.

## S17-SRC-005

- **Title/repository:** AMD APU ASIC information table — Linux kernel
- **URL:** https://github.com/torvalds/linux/blob/fce2dfa773ced15f27dd27cd0b482a7473cdcf2a/Documentation/gpu/amdgpu/apu-asic-info-table.csv
- **Revision/date:** commit `fce2dfa773ced15f27dd27cd0b482a7473cdcf2a`, master observed 2026-07-16; accessed 2026-07-16
- **Supports:** Strix Halo DCN/GC/VCN/SDMA/MP0/MP1 IP versions.
- **Limitations:** family-level documentation; does not establish installed kernel/firmware support, enabled blocks, or performance.

## S17-SRC-006

- **Title/repository:** RADV driver documentation — Mesa
- **URL:** https://gitlab.freedesktop.org/mesa/mesa/-/blob/20f4f9f45057559475600b60364b60643011990f/docs/drivers/radv.rst
- **Revision/date:** commit `20f4f9f45057559475600b60364b60643011990f`, main observed 2026-07-16; accessed 2026-07-16
- **Supports:** RADV supports graphics-capable RDNA GPUs supported by Linux; GFX8+ Vulkan 1.4 policy; RADV/kernel responsibility split.
- **Limitations:** general driver policy, not a Radeon 8060S conformance, extension, performance, or installed-stack report.

## S17-SRC-007

- **Title/publisher:** How to accelerate AI applications on RDNA 3 using WMMA — AMD GPUOpen
- **URL:** https://gpuopen.com/learn/wmma_on_rdna3/
- **Revision/date:** published 2023-01-10; accessed 2026-07-16
- **Supports:** RDNA 3 WMMA model and documented FP16/BF16/I8/I4 input formats.
- **Limitations:** demonstrates RDNA 3, not specifically RDNA 3.5/`gfx1151`; does not prove installed compiler/library support or HaloFPX correctness/performance.

## S17-SRC-008

- **Title/publisher:** ROCm 7.1.1 release notes — AMD ROCm documentation
- **URL:** https://rocm.docs.amd.com/en/docs-7.1.1/about/release-notes.html
- **Revision/date:** ROCm 7.1.1, rocWMMA 2.1.0 release-note entry; accessed 2026-07-16
- **Supports:** rocWMMA built with `gfx1151` target for ROCm 7.0 and later.
- **Limitations:** build-target inclusion is not per-operation support or a correctness/performance guarantee.

## Conflict and freshness notes

- **[VERIFIED]** S17-SRC-002, S17-SRC-003, and pinned S17-SRC-004 agree on Radeon 8060S as `gfx1151`.
- **[RECOMMENDATION]** Re-verify volatile AMD/ROCm live pages at each stack freeze. Preserve local snapshots under `sources/` only through the project import/provenance process; this section contains summaries and stable links, not copied source bodies.
