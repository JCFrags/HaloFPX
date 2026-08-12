---
section_id: "26"
title: "Toolchain design implications"
status: "needs-machine-validation"
last_verified: "2026-07-17"
applies_to:
  repositories: ["charlie12345/ROCmFPX", "ggml-org/llama.cpp"]
  software_versions: ["ROCm 7.2.3 research comparison", "CMake 4.4.0 documentation comparison"]
  hardware_revisions: ["gfx1151 Strix Halo"]
related_sections: ["24", "25", "27", "28"]
---

# Toolchain design implications

## Build identity is a runtime compatibility key

**[RECOMMENDATION]** Embed a machine-readable build manifest in each executable and package. At minimum include:

```text
schema, project commit, dirty flag, dependency commits,
ROCm/HIP version, clang/LLVM version, linker/version,
CMake/Ninja/Python versions, build preset, compile/link flags,
GPU targets, SOURCE_DATE_EPOCH, package-lock digest, build ID
```

Expose it with `--build-info=json` and attach the same JSON to experiment records. **[INFERENCE]** This prevents two hosts with nominally identical application versions but different device code or runtime libraries from being treated as a matched distributed rank pair.

## Accepted build matrix

| Lane | Purpose | Publication rule |
|---|---|---|
| release | User-facing baseline | Correctness suite, two-host smoke test, manifest present |
| relwithdebinfo | Profiling | Same workload correctness; symbols/build IDs archived |
| CPU ASan+UBSan | Host memory/UB detection | No device-performance claims |
| HIP debug | Kernel/runtime diagnosis | Record any changed optimization and timing behavior |
| LTO candidate | Cross-TU optimization | Promote only after matched correctness and performance evidence |
| PGO candidate | Workload-guided optimization | Profile corpus and merge procedure must be versioned |

**[RECOMMENDATION]** Never make LTO or PGO the only build path. Keep a known-debuggable non-LTO baseline so regressions can be bisected.

## Dependency and generated-artifact policy

- **[RECOMMENDATION]** Lock Git dependencies by full commit and verify fetched content hashes; preserve license metadata.
- **[RECOMMENDATION]** Treat generated kernels, SPIR-V, code objects, embedded assets, and pipeline caches as derived artifacts with generator version, input digest, and regeneration command.
- **[RECOMMENDATION]** Reject startup-time consumption of an incompatible pipeline/KV/cache artifact rather than accepting it. Cache corruption must be a miss, consistent with project governance.
- **[OPEN]** Whether ROCmFPX and upstream llama.cpp can share a single preset hierarchy without carrying fork-only options needs a source-level option inventory.

## Distributed fallback

**[RECOMMENDATION]** The binary manifest must state rank role and compatibility fingerprint, but a manifest mismatch must fail before model execution and preserve a single-node fallback. Build identity does not replace protocol negotiation.
