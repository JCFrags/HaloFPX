---
section_id: "17"
title: "AMD Strix Halo SoC and gfx1151 Architecture"
status: "needs-machine-validation"
last_verified: "2026-07-16"
applies_to:
  repositories: []
  software_versions: ["ROCm 7.14.0", "ROCm 7.1.1", "LLVM main c571b0bd7330a4b737ad7dec31e7f2b52edd3953", "Linux fce2dfa773ced15f27dd27cd0b482a7473cdcf2a", "Mesa main 20f4f9f45057559475600b60364b60643011990f"]
  hardware_revisions: ["AMD Ryzen AI Max+ 395 / Radeon 8060S; exact project machines OPEN"]
related_sections: ["18", "19", "20", "22", "23", "24", "25", "37", "74"]
---

# AMD Strix Halo SoC and gfx1151 Architecture

## High-value orientation

- **[VERIFIED]** AMD identifies Ryzen AI Max+ 395 as Strix Halo: 16 Zen 5 cores/32 threads, Radeon 8060S with 40 graphics cores, a 256-bit LPDDR5X-8000 interface supporting up to 128 GB, and a 45–120 W configurable TDP range ([S17-SRC-001](sources.md#s17-src-001)).
- **[VERIFIED]** AMD's ROCm 7.14.0 compatibility matrix maps the consumer Ryzen AI Max+ 395 and Radeon 8060S family to `gfx1151`; its GPU table classifies the closely corresponding PRO 395/8060S as RDNA 3.5 with 40 CUs ([S17-SRC-002](sources.md#s17-src-002), [S17-SRC-003](sources.md#s17-src-003)).
- **[VERIFIED]** The pinned LLVM source maps Radeon 8060S to the `gfx1151` APU target. Target-specific code objects are therefore the safe baseline; names such as `gfx11` or `RDNA 3.5` are not interchangeable binary compatibility promises ([S17-SRC-004](sources.md#s17-src-004)).
- **[INFERENCE]** HaloFPX will usually be constrained by shared-memory bandwidth, capacity allocation, kernel quality, and sustained package power before nominal CPU/GPU/NPU peak rates. This follows from CPU and GPU sharing the package and 256-bit LPDDR5X interface; it is not a measured bottleneck on either project machine.
- **[OPEN]** Exact CPU CCD/CCX topology, firmware-configured GPU carveout, usable HSA/Vulkan heaps, installed target identity, enabled matrix instructions, memory bandwidth, power policy, and thermal sustainment are machine-specific. Resolve them with [S17-EXP-001 through S17-EXP-005](procedures_and_checks.md#machine-experiment-register).

## Page map

- [Facts and constraints](facts_and_constraints.md) — cited architecture and support facts.
- [Design implications](design_implications.md) — consequences for HaloFPX.
- [Procedures and checks](procedures_and_checks.md) — exact non-destructive host tests.
- [Open questions](open_questions.md) — unresolved evidence and decisions.
- [Sources](sources.md) — stable source records and limitations.

## Research split

1. **Internet/source-code research complete now:** product specifications, `gfx1151` identity, ROCm listing, GPU cache/wavefront values, Linux IP versions, and RADV support policy.
2. **Actual-machine work required:** topology, heap and carveout sizes, instruction/library exposure, Vulkan properties, bandwidth, and sustained clocks/power.
3. **Contingent decisions:** kernel variants, HIP versus Vulkan preference, CPU thread placement, memory budgets, and power profile remain undecided until matched experiments exist.

## Scope boundary

**[RECOMMENDATION]** Treat this page as the silicon and compiler-target orientation only. Exact BOM/firmware belongs to section 18; allocation policy to 19; USB4 topology to 20; power/thermals to 22; stack compatibility to 23; HIP/HSA and Vulkan semantics to 24/25; kernel optimization to 37; and benchmark results to 74.
