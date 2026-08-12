# Building ROCmFP4 for AMD GPUs

ROCmFP4 runs on **CPU**, **Vulkan**, and **ROCm/HIP**. Most users want the HIP
backend for best performance. To build HIP support, you must compile for your
GPU's `gfx` target.

Vulkan does not need a `gfx` target at compile time and works across RDNA2+ AMD
GPUs, but HIP performance tuning in this tree is strongest on Strix Halo.

---

## Choose Your Build Path

| Your GPU | Example cards | Easiest build command | Build output folder |
|---|---|---|---|
| **Strix Halo / RDNA3.5** | Ryzen AI MAX+ 395, Framework Desktop | `scripts/build-strix-rocmfp4-mtp.sh` | `build-strix-rocmfp4/` |
| **RDNA2** | RX 6800 / RX 6900 | `scripts/build-rdna2.sh` (`gfx1030` default) | `build-rdna2/` |
| **RDNA2** | RX 6700 / RX 6600 | set `gfx1031` / `gfx1032` as shown below | `build-rdna2/` |
| **RDNA3** | RX 7900 XTX | `scripts/build-rdna3.sh` (`gfx1100` default) | `build-rdna3/` |
| **RDNA3** | RX 7800 XT / RX 7600 | set `gfx1101` / `gfx1102` as shown below | `build-rdna3/` |
| **RDNA4** | RX 9070 XT | `scripts/build-rdna4.sh` (`gfx1201` default) | `build-rdna4/` |
| **RDNA4** | RX 9060 XT | set `gfx1200` as shown below | `build-rdna4/` |
| **Vega 20 / gfx906 experimental** | Radeon Instinct MI50 / MI60 | `scripts/build-gfx906.sh` | `build-gfx906/` |
| **Windows HIP** | supported AMD GPUs on Windows | [OPEN] no tracked, validated wrapper in this repository | custom |
| **Vulkan only** | Any AMD GPU with Vulkan drivers | see [Vulkan-only build](#vulkan-only-no-hip-arch-needed) | `build-vulkan/` |

All Linux scripts accept `JOBS=16` to control parallel compile jobs:

```bash
env JOBS=16 scripts/build-rdna3.sh
```

---

## Find Your `gfx` Target

On Linux, check what ROCm reports:

```bash
rocminfo | grep -m1 "Name:"
rocminfo | grep -m1 "gfx"
```

Cross-check the result against AMD's current
[GPU specifications](https://rocm.docs.amd.com/en/latest/reference/gpu-specs.html);
the exact LLVM target, not only the RDNA generation name, selects the build.

Then match your GPU to this table:

| AMD generation | Example hardware | Typical `gfx` IDs | Build target | Linux runtime fallback |
|---|---|---|---|---|
| Vega 20 / GCN5 | Radeon Instinct MI50 / MI60 | `gfx906` | `gfx906` | use native `gfx906` when ROCm supports it |
| RDNA1 | RX 5700/5600; RX 5500 class | `gfx1010`; `gfx1012` | exact reported target | any override is runtime-only and requires validation |
| RDNA2 | RX 6800/6900; RX 6700; RX 6600 | `gfx1030`; `gfx1031`; `gfx1032` (family range through `gfx1036`) | exact reported target | any override is runtime-only and is not a substitute for an exact-target build |
| RDNA3 | RX 7900 XTX/XT/GRE; RX 7800/7700; RX 7600 | `gfx1100`; `gfx1101`; `gfx1102` | exact reported target | do not substitute an override for an exact-target build without testing |
| RDNA3.5 | Strix Point Ryzen AI 300; Strix Halo Ryzen AI MAX | `gfx1150`; `gfx1151` | exact reported target; this project defaults to Strix Halo `gfx1151` | any override is runtime-only and requires validation |
| RDNA4 | RX 9070 XT/9070/GRE; RX 9060 XT/9060 | `gfx1201`; `gfx1200` | exact reported target | use native `gfx` when ROCm supports it |

**Tips**

- Build the exact target reported for the GPU when the installed ROCm release
  supports it. A generation-level fallback is not evidence of compatibility.
- Published benchmark numbers and regression guards assume **Strix Halo /
  `gfx1151`**.
- Vega 20 / `gfx906` is an experimental community target. It is not RDNA/CDNA,
  and should be validated on real MI50/MI60 hardware before claiming support.
- `HSA_OVERRIDE_GFX_VERSION` works on **Linux only** — not on Windows.

---

## Build Scripts

This repository provides one generic builder plus thin wrappers per generation.
You do not need separate full build scripts for each architecture.

| Script | Target | Notes |
|---|---|---|
| `scripts/build-strix-rocmfp4-mtp.sh` | `gfx1151` | Validated default; includes regression-test binaries |
| `scripts/build-rdna2.sh` | `gfx1030` default; override supported | RX 6800/6900 default; use `gfx1031` for RX 6700 and `gfx1032` for RX 6600 |
| `scripts/build-rdna3.sh` | `gfx1100` default; override supported | RX 7900 defaults; use `gfx1101` for RX 7800/7700 and `gfx1102` for RX 7600 |
| `scripts/build-rdna4.sh` | `gfx1201` default; override supported | RX 9070 defaults; use `gfx1200` for RX 9060 |
| `scripts/build-gfx906.sh` | `gfx906` | Experimental Vega 20 / MI50 / MI60 community target |
| `scripts/build-rocmfp4.sh` | any `gfx` | Generic — set `CMAKE_HIP_ARCHITECTURES` yourself |

Generic example (any single target):

```bash
env CMAKE_HIP_ARCHITECTURES=gfx1100 BUILD_DIR=build-rdna3 scripts/build-rocmfp4.sh
```

---

## Common CMake Flags

Every ROCmFP4 HIP build in this tree uses:

| Flag | Value | Why |
|---|---|---|
| `GGML_HIP` | `ON` | Enable ROCm/HIP backend |
| `GGML_VULKAN` | `ON` | Enable Vulkan (recommended fallback) |
| `GGML_CUDA` | `OFF` | Disable NVIDIA CUDA |
| `GGML_HIP_FORCE_MMQ` | `ON` | Required for ROCmFP4 MMQ kernels |
| `CMAKE_BUILD_TYPE` | `Release` | Release build |
| `CMAKE_HIP_ARCHITECTURES` or `GPU_TARGETS` | your `gfx` | GPU ISA to compile for |

The repository wrappers accept the environment variable
`CMAKE_HIP_ARCHITECTURES`; they do not consume an environment variable named
`GPU_TARGETS`. A direct CMake configuration can pass either
`-DGPU_TARGETS=...` or `-DCMAKE_HIP_ARCHITECTURES=...`. Do not set only
environment `GPU_TARGETS` when invoking a wrapper.

---

## Per-Architecture Commands

### Strix Halo / RDNA3.5 (validated default)

```bash
env JOBS=16 scripts/build-strix-rocmfp4-mtp.sh
```

Run with:

```bash
HSA_OVERRIDE_GFX_VERSION=11.5.1 \
GGML_HIP_ENABLE_UNIFIED_MEMORY=1 \
./build-strix-rocmfp4/bin/llama-cli -m model.gguf -dev ROCm0 -ngl 999 ...
```

Full Strix install guide: [`docs/STRIX-HALO-QUICKSTART.md`](STRIX-HALO-QUICKSTART.md)

### RDNA2 — Linux

```bash
env JOBS=16 scripts/build-rdna2.sh
env JOBS=16 CMAKE_HIP_ARCHITECTURES=gfx1031 scripts/build-rdna2.sh     # RX 6700
env JOBS=16 CMAKE_HIP_ARCHITECTURES=gfx1032 scripts/build-rdna2.sh     # RX 6600
```

The following override is a runtime experiment only; it does not make a binary
compiled for a different code object an exact-target build:

```bash
HSA_OVERRIDE_GFX_VERSION=10.3.0 ./build-rdna2/bin/llama-cli -m model.gguf -dev ROCm0 ...
```

### RDNA3 — Linux

```bash
env JOBS=16 scripts/build-rdna3.sh                                      # RX 7900: gfx1100
env JOBS=16 CMAKE_HIP_ARCHITECTURES=gfx1101 scripts/build-rdna3.sh     # RX 7800/7700
env JOBS=16 CMAKE_HIP_ARCHITECTURES=gfx1102 scripts/build-rdna3.sh     # RX 7600
```

### RDNA4 — Linux

```bash
env JOBS=16 scripts/build-rdna4.sh                                      # RX 9070: gfx1201
env JOBS=16 CMAKE_HIP_ARCHITECTURES=gfx1200 scripts/build-rdna4.sh     # RX 9060
```

Requires a ROCm version with the selected `gfx1200` or `gfx1201` device
libraries. If HIP is not ready yet,
use the [Vulkan-only path](#vulkan-only-no-hip-arch-needed).

### Vega 20 / gfx906 — Linux Experimental

```bash
env JOBS=16 scripts/build-gfx906.sh
```

This target is intended for community testing on Radeon Instinct MI50 / MI60
hardware. It is additive and does not change the RDNA2/RDNA3/Strix/RDNA4 build
defaults.

Minimum validation before reporting it as working:

```bash
./build-gfx906/bin/test-backend-ops -b ROCm0
./build-gfx906/bin/test-quantize-fns
./build-gfx906/bin/llama-bench -m model.gguf -dev ROCm0 -ngl 999
```

If HIP support is unreliable on a specific ROCm version, try the
[Vulkan-only path](#vulkan-only-no-hip-arch-needed) first.

### Windows

[OPEN] This repository does not contain a validated Windows HIP wrapper. Do not
follow references to `build-hip.bat`; no such tracked file exists. Use a ROCm
for Windows release that supports the exact GPU, record the CMake generator and
toolchain, and compile for the exact `gfx` target reported by that environment.
The Linux wrappers above have not been validated as Windows recipes.

`HSA_OVERRIDE_GFX_VERSION` does not work on Windows. A future Windows wrapper
must expose exact-target selection (`gfx1100`, `gfx1101`, or `gfx1102` for the
corresponding RDNA3 card) and pass a hardware smoke gate before this lane can be
promoted from [OPEN].

---

## Multi-GPU / Distribution Build

Build one binary for several AMD GPUs by listing targets separated by semicolons.
Compile time and binary size increase significantly.

```bash
cmake -S . -B build-multi \
  -DCMAKE_BUILD_TYPE=Release \
  -DGGML_HIP=ON \
  -DGGML_HIP_FORCE_MMQ=ON \
  -DGGML_VULKAN=ON \
  -DGGML_CUDA=OFF \
  -DGGML_NATIVE=OFF \
  -DGPU_TARGETS="gfx1030;gfx1031;gfx1032;gfx1100;gfx1101;gfx1102;gfx1150;gfx1151;gfx1200;gfx1201"

cmake --build build-multi -j "$(nproc)"
```

| Use case | `GPU_TARGETS` |
|---|---|
| RDNA2 only | `gfx1030;gfx1031;gfx1032` |
| RDNA3 only | `gfx1100;gfx1101;gfx1102` |
| RDNA3 + Strix Halo | `gfx1100;gfx1101;gfx1102;gfx1151` |
| Project-documented AMD targets | `gfx1030;gfx1031;gfx1032;gfx1100;gfx1101;gfx1102;gfx1150;gfx1151;gfx1200;gfx1201` |
| Experimental MI50/MI60 add-on | append `gfx906` only if you intend to test Vega 20 |

---

## Runtime Environment Variables

| Variable | When to use | Example |
|---|---|---|
| `HSA_OVERRIDE_GFX_VERSION` | Linux; GPU not in official ROCm support | `10.3.0`, `11.0.0`, `11.5.1` |
| `GGML_HIP_ENABLE_UNIFIED_MEMORY` | UMA systems (Strix Halo, APUs) | `1` |
| `HIP_VISIBLE_DEVICES` | Pick a specific GPU | `0` |

---

## Vulkan Only (No HIP Arch Needed)

Use this when ROCm/HIP is unavailable or your GPU is not yet supported by HIP:

```bash
cmake -S . -B build-vulkan \
  -DCMAKE_BUILD_TYPE=Release \
  -DGGML_VULKAN=ON \
  -DGGML_HIP=OFF \
  -DGGML_CUDA=OFF

cmake --build build-vulkan -j "$(nproc)" --target llama-cli llama-quantize

./build-vulkan/bin/llama-cli -m model.gguf -dev Vulkan0 -ngl 999 ...
```

---

## Advanced Tuning (Experts Only)

HIP micro-optimization knobs are passed via `CMAKE_HIP_FLAGS`. Defaults are
already tuned — only change these if you are running regression guards.

```bash
env CMAKE_HIP_FLAGS="-DGGML_ROCMFP4_UNALIGNED_QS_DWORD_LOAD=0" \
    CMAKE_HIP_ARCHITECTURES=gfx1151 \
    scripts/build-rocmfp4.sh
```

Full list of knobs: [`ggml/rocmfp4/README.md`](../ggml/rocmfp4/README.md)

---

## Validate Your Build

```bash
# CPU quant check (no GPU needed)
scripts/check-rocmfp4-quant-regression.sh

# Full gate (Strix defaults — override BUILD_DIR for other builds)
env HSA_OVERRIDE_GFX_VERSION=11.5.1 BUILD_DIR=build-strix-rocmfp4 \
    scripts/check-rocmfp4-all-regression.sh
```

Set `BUILD_DIR`, `BIN`, or `TEST_BACKEND_OPS_BIN` when not using the Strix
default paths. Details: [`docs/ROCmFP4-REPRODUCIBILITY.md`](ROCmFP4-REPRODUCIBILITY.md)

---

## Quick Reference

```bash
# Strix Halo (best-tested path)
env JOBS=16 scripts/build-strix-rocmfp4-mtp.sh

# Desktop AMD GPUs
env JOBS=16 scripts/build-rdna2.sh   # RX 6000
env JOBS=16 CMAKE_HIP_ARCHITECTURES=gfx1031 scripts/build-rdna2.sh     # RX 6700
env JOBS=16 CMAKE_HIP_ARCHITECTURES=gfx1032 scripts/build-rdna2.sh     # RX 6600
env JOBS=16 scripts/build-rdna3.sh                                      # RX 7900 (gfx1100)
env JOBS=16 CMAKE_HIP_ARCHITECTURES=gfx1101 scripts/build-rdna3.sh     # RX 7800/7700
env JOBS=16 CMAKE_HIP_ARCHITECTURES=gfx1102 scripts/build-rdna3.sh     # RX 7600
env JOBS=16 scripts/build-rdna4.sh                                      # RX 9070 (gfx1201)
env JOBS=16 CMAKE_HIP_ARCHITECTURES=gfx1200 scripts/build-rdna4.sh     # RX 9060

# Windows HIP: [OPEN] no validated wrapper is tracked
```
