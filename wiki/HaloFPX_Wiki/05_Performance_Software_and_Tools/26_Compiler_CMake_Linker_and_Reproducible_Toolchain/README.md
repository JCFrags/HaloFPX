---
section_id: "26"
title: "Compiler, CMake, Linker, and Reproducible Toolchain"
status: "needs-machine-validation"
last_verified: "2026-07-17"
applies_to:
  repositories: ["charlie12345/ROCmFPX", "ggml-org/llama.cpp", "fewtarius/CachyLLama"]
  software_versions: ["ROCm 7.2.3 research comparison", "CMake 4.4.0 documentation comparison"]
  hardware_revisions: ["gfx1151 Strix Halo"]
related_sections: ["24", "25", "27", "28"]
---

# Compiler, CMake, Linker, and Reproducible Toolchain

## Decision summary

**[VERIFIED]** AMD's immutable ROCm 7.2.3 release notes identify the 2026-05-04 release and its component versions, while AMD's versioned Strix Halo guidance describes the `gfx1151` platform. CMake 4.4.0 documentation is an inspected mechanism reference, not the selected project build version. [S26-01][S26-02][S26-03]

**[RECOMMENDATION]** Establish one lock file per accepted build containing the ROCm package set, compiler/linker identities, CMake/Ninja/Python versions, dependency commits, kernel/driver identity, configure cache, architecture flags, and source tree status. A version name such as "ROCm latest" is not a reproducible toolchain.

**[OPEN]** ROCm 7.2.3 is only a coherent research comparison with section 24, not an approved deployment baseline. No exact OS/package, Clang/LLVM, GCC, HIP compiler, shader compiler, CMake, Ninja, Python, linker, or packaging tuple has been selected or inspected on both project machines. No clean build, deterministic rebuild comparison, sanitizer run, LTO/PGO comparison, or `gfx1151` binary inspection has been performed.

## Authoritative pages

- [Facts and constraints](facts_and_constraints.md)
- [Design implications](design_implications.md)
- [Procedures and checks](procedures_and_checks.md)
- [Open questions](open_questions.md)
- [Source records](sources.md)

## Research split

1. Internet/source research establishes supported mechanisms and candidate pinned inputs.
2. The two Strix Halo hosts must prove package availability, emitted targets, runtime loading, reproducibility, and performance.
3. LTO, PGO, linker, sanitizer, and packaging choices remain contingent on matched-host experiments.
