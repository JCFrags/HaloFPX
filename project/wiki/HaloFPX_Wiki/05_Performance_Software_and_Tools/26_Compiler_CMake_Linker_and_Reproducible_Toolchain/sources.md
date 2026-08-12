---
section_id: "26"
title: "Toolchain sources"
status: "draft"
last_verified: "2026-07-17"
applies_to:
  repositories: ["charlie12345/ROCmFPX", "ggml-org/llama.cpp", "fewtarius/CachyLLama"]
  software_versions: ["ROCm 7.2.3 research comparison", "CMake 4.4.0 documentation comparison"]
  hardware_revisions: ["gfx1151 Strix Halo"]
related_sections: ["24", "25", "27"]
---

# Toolchain sources

Access date for all Internet records: 2026-07-16.

| ID | Primary source and revision | Supports | Limitations |
|---|---|---|---|
| S26-01 | AMD, [ROCm 7.2.3 release notes](https://rocm.docs.amd.com/en/docs-7.2.3/about/release-notes.html), released 2026-05-04 | immutable ROCm release identity and coordinated component versions | Release documentation does not select or inventory the project hosts. |
| S26-02 | AMD, [Strix Halo system optimization](https://rocm.docs.amd.com/en/docs-7.2.0/how-to/system-optimization/strixhalo.html), ROCm 7.2 documentation, page dated 2026-02-20 | official `gfx1151`/Strix Halo platform guidance | Guidance is not an exact supported package tuple or successful HaloFPX build. |
| S26-03 | Kitware, [cmake-presets(7)](https://cmake.org/cmake/help/v4.4/manual/cmake-presets.7.html), CMake 4.4.0 | Preset files, sharing model, schema | Does not define HaloFPX presets. |
| S26-04 | Kitware, [CMAKE_HIP_ARCHITECTURES](https://cmake.org/cmake/help/v4.4/variable/CMAKE_HIP_ARCHITECTURES.html), CMake 4.4.0 | HIP target initialization | Emitted object must still be inspected. |
| S26-05 | Kitware, [BUILD_RPATH_USE_ORIGIN](https://cmake.org/cmake/help/v4.4/prop_tgt/BUILD_RPATH_USE_ORIGIN.html), CMake 4.4.0 | Reproducible relative build-tree RPATH | Only one nondeterminism source. |
| S26-06 | Kitware, [string command](https://cmake.org/cmake/help/v4.4/command/string.html), CMake 4.4.0 | `SOURCE_DATE_EPOCH` use for timestamps | Toolchain-wide compliance not guaranteed. |
| S26-07 | `charlie12345/ROCmFPX`, [commit `a5605a7`](https://github.com/charlie12345/ROCmFPX/commit/a5605a72768c6562241b248e268e33dc92787394), observed repository head | Exact observed source state | Not an approved or immutable project baseline until mirrored/pinned. |
| S26-08 | `ggml-org/llama.cpp`, [commit `788e07d`](https://github.com/ggml-org/llama.cpp/commit/788e07dc91d266ad3162a1ce9037665656269689), observed repository head | Exact observed upstream state | Same limitation. |
| S26-09 | `fewtarius/CachyLLama`, [commit `6be7459`](https://github.com/fewtarius/CachyLLama/commit/6be745998f568e379ea197fcf827baec73ff9940), observed repository head | Exact observed cache-fork state | Same limitation. |
| S26-10 | Kitware, [CheckIPOSupported](https://cmake.org/cmake/help/v4.4/module/CheckIPOSupported.html), CMake 4.4.0 | IPO/LTO capability gate | Successful check does not prove runtime benefit or correctness. |
| S26-11 | Reproducible Builds, [SOURCE_DATE_EPOCH specification](https://reproducible-builds.org/specs/source-date-epoch/), current specification accessed 2026-07-16 | Normalized build timestamp convention | A convention, not complete reproducibility. |

## Conflicts and freshness

**[VERIFIED]** The previously recorded `ROCm Core SDK 7.14.0` candidate was not present in AMD's official release history when rechecked on 2026-07-17 and has been removed. ROCm 7.2.3 is retained only as an immutable comparison release aligned with section 24. **[OPEN]** Recheck exact compatibility and package metadata on both hosts before selecting any executable baseline; keep future `latest` or `develop` snapshots explicitly separate from release evidence.
