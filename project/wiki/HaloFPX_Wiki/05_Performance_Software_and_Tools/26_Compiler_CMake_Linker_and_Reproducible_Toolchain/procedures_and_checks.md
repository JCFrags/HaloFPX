---
section_id: "26"
title: "Toolchain procedures and checks"
status: "needs-machine-validation"
last_verified: "2026-07-17"
applies_to:
  repositories: ["charlie12345/ROCmFPX", "ggml-org/llama.cpp"]
  software_versions: ["ROCm 7.2.3 research comparison", "CMake 4.4.0 documentation comparison"]
  hardware_revisions: ["gfx1151 Strix Halo"]
related_sections: ["24", "25", "27", "28"]
---

# Toolchain procedures and checks

All commands are non-destructive unless noted. Run on each Linux node from a clean worktree; root is not required for inventory/build, but package installation is outside this procedure.

## 1. Capture toolchain identity

```bash
set -eu
mkdir -p evidence/toolchain
date -u +%FT%TZ > evidence/toolchain/captured_at_utc.txt
git rev-parse HEAD > evidence/toolchain/source_commit.txt
git status --porcelain=v1 > evidence/toolchain/source_status.txt
cmake --version > evidence/toolchain/cmake.txt
ninja --version > evidence/toolchain/ninja.txt
python3 --version > evidence/toolchain/python.txt 2>&1
clang++ --version > evidence/toolchain/clang.txt
hipcc --version > evidence/toolchain/hipcc.txt 2>&1
ld.lld --version > evidence/toolchain/lld.txt
rocminfo > evidence/toolchain/rocminfo.txt
```

Also save the distribution package manifest, `/sys/module/amdgpu/version` when present, kernel command line, `uname -a`, and hashes of compiler/linker executables. **[RECOMMENDATION]** Redact usernames/paths only in a derived public copy; retain the raw private evidence.

## 2. Configure and inspect

The tracked configure preset is `strix-rocmfpx`; there is no build preset named
`release-gfx1151`. From the combined repository root, use the actual configure
preset and its output directory:

```bash
export SOURCE_DATE_EPOCH="$(git log -1 --pretty=%ct)"
cmake --preset strix-rocmfpx
cmake --build build-strix-rocmfpx --verbose
cmake --build build-strix-rocmfpx --target help > evidence/toolchain/targets.txt
```

Preserve `CMakeCache.txt`, `CMakeConfigureLog.yaml`, `compile_commands.json`,
and the expanded build command. A future deterministic release preset remains
[OPEN]; do not advertise it until it is tracked and validated.

```bash
find build-strix-rocmfpx -type f -perm -u+x -exec file {} \; > evidence/toolchain/file.txt
readelf -n path/to/binary > evidence/toolchain/notes.txt
readelf -d path/to/binary > evidence/toolchain/dynamic.txt
llvm-objdump --offloading path/to/binary > evidence/toolchain/offloading.txt
```

**[OPEN]** Confirm the correct ROCm utility for enumerating bundled AMDGPU code objects in this SDK; do not treat a successful build as proof it targets `gfx1151`.

## 3. Deterministic rebuild test

Build twice from the same clean source into two different absolute paths, with identical environment/container and `SOURCE_DATE_EPOCH`. Hash every installed artifact and compare manifests.

```bash
sha256sum install-a/**/* 2>/dev/null | sort -k2 > evidence/hashes-a.txt
sha256sum install-b/**/* 2>/dev/null | sort -k2 > evidence/hashes-b.txt
diff -u evidence/hashes-a.txt evidence/hashes-b.txt
```

If hashes differ, use `diffoscope`; record the cause rather than relabeling the build reproducible.

## 4. Optimization qualification

For baseline, LTO and PGO builds, use the identical model, prompt corpus, rank topology, thermal state and runtime settings. Run correctness first; then collect section 27 profiles and repeated latency/throughput samples. **[MEASURED]** may be used only when raw results and environment manifests are checked in under `experiments/`.

## 5. Two-host compatibility gate

Before connecting ranks, compare the manifest schema, application and dependency commits, ABI/protocol version, GPU target, model/tokenizer hashes, runtime/KMD compatibility tuple, and transport build options. On mismatch, stop distributed startup and offer the documented single-node mode.
