---
section_id: "26"
title: "Toolchain facts and constraints"
status: "needs-machine-validation"
last_verified: "2026-07-17"
applies_to:
  repositories: ["charlie12345/ROCmFPX", "ggml-org/llama.cpp", "fewtarius/CachyLLama"]
  software_versions: ["ROCm 7.2.3 research comparison", "CMake 4.4.0 documentation comparison"]
  hardware_revisions: ["gfx1151 Strix Halo"]
related_sections: ["24", "25", "27"]
---

# Toolchain facts and constraints

| Area | Evidence-backed constraint | HaloFPX status |
|---|---|---|
| HIP compiler | **[VERIFIED]** ROCm 7.2.3 is a coordinated release with versioned components; AMD publishes separate Strix Halo guidance for `gfx1151`. [S26-01][S26-02] | **[OPEN]** Select and inventory the exact supported OS/package/compiler tuple on both hosts; the release page is not proof of a successful build. |
| Target architecture | **[VERIFIED]** HIP/CMake accepts GPU architecture selection through the HIP language/toolchain; emitted code still must be inspected. [S26-04] | **[OPEN]** Prove `gfx1151` code objects on both nodes. |
| CMake | **[VERIFIED]** `CMakePresets.json` is project-shareable; `CMakeUserPresets.json` is local. Presets were added in 3.19. [S26-03] | Put accepted presets in source control; never rely on an unrecorded cache. |
| CMake reproducibility | **[VERIFIED]** `BUILD_RPATH_USE_ORIGIN` removes build-tree absolute paths from RPATH, and CMake date operations honor `SOURCE_DATE_EPOCH`. [S26-05][S26-06] | Necessary controls, not proof of identical output. |
| Ninja | **[INFERENCE]** A pinned Ninja generator reduces generator variation and exposes parallel build commands clearly, but it does not make compiler output deterministic. | Record executable hash and version. |
| C/C++ compiler | **[RECOMMENDATION]** Use the Clang/LLVM supplied with the selected ROCm package for HIP and test the same Clang for host C++; keep GCC only as a compatibility lane until ABI/link behavior is proven. | Exact versions pending host inventory. |
| Linker | **[RECOMMENDATION]** Start with the ROCm/LLVM-matched `ld.lld`; maintain a `bfd` fallback only if a dependency requires it. | Compare symbols, loadability, size, startup, and throughput. |
| Vulkan shader compiler | **[RECOMMENDATION]** Pin Mesa/RADV plus the exact shader compilation path used by the Vulkan backend; capture SPIR-V and pipeline-cache provenance separately from host binaries. | Depends on section 25. |
| Python | **[RECOMMENDATION]** Pin interpreter major/minor, resolved wheels/sdists and hashes in an isolated environment. | Do not infer Python support from the OS default. |
| Dependency state | **[VERIFIED]** The inspected heads were `ROCmFPX` `a5605a72768c6562241b248e268e33dc92787394`, `llama.cpp` `788e07dc91d266ad3162a1ce9037665656269689`, and `CachyLLama` `6be745998f568e379ea197fcf827baec73ff9940` at access time. [S26-07][S26-08][S26-09] | These are observations, not approved baselines; submodules and dirty state still matter. |

## Required toolchain selection matrix

No row below is an approved HaloFPX version. The CMake 4.4 pages are mechanism references only.

| Required component | Current evidence | Selection state |
|---|---|---|
| Clang/LLVM and HIP compiler | coordinated ROCm release evidence only | **[OPEN]** exact executables, hashes, targets, and compatibility |
| GCC host compatibility lane | no selected version | **[OPEN]** supported range and ABI/link comparison |
| Vulkan shader compiler / Mesa ACO path | delegated to section 25 | **[OPEN]** installed Mesa/compiler lineage and generated artifact fingerprint |
| CMake | 4.4.0 versioned documentation inspected | **[OPEN]** minimum and selected executable/hash |
| Ninja | workflow recommendation only | **[OPEN]** selected executable/hash |
| Python and packages | repository-dependent | **[OPEN]** interpreter range and hashed environment lock |
| LLD/BFD and system linker inputs | candidate recommendation only | **[OPEN]** selected linker, ABI, symbols, and runtime loading |
| OS/package manager and package set | no host inventory | **[OPEN]** exact repository URLs, package NEVR/version, hashes, and licenses |

## Optimization and diagnostic builds

- **[VERIFIED]** CMake supports interprocedural optimization through target properties and compiler capability checks. [S26-10]
- **[RECOMMENDATION]** Maintain distinct `release`, `relwithdebinfo`, `asan-ubsan-cpu`, `debug-hip`, `lto-candidate`, and `pgo-candidate` presets. Do not combine validation and optimization conclusions from different binaries.
- **[RECOMMENDATION]** Retain split debug symbols and build IDs for every benchmarked binary and preserve unstripped artifacts privately.
- **[OPEN]** GPU sanitizer/debug support and false-positive behavior on this exact ROCm/gfx1151 stack require machine validation; CPU sanitizers do not validate device kernels.

## Reproducibility boundary

**[INFERENCE]** Bit-identical host binaries may be achievable after normalizing time, paths, archive ordering, locale, and debug prefixes. GPU code objects, packaged shared libraries, and generated pipeline caches may add nondeterminism. Reproducibility must be asserted per artifact, not per build directory.
