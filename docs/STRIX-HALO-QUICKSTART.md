# Strix Halo Quickstart

This guide has two distinct lanes. The HaloFPX performance targets are the two
Nimo Direct MME3L machines running CachyOS. Ubuntu/Framework instructions are
portability or donor-history guidance and are not evidence about the installed
targets. Read the dated
[`TARGET_MACHINES.md`](../project/TARGET_MACHINES.md) before changing either
node.

Strix Halo / RDNA3.5 (`gfx1151`) install guide. For other AMD GPUs (RDNA2,
RDNA3, RDNA4), see [`BUILD-AMD-ARCHITECTURES.md`](BUILD-AMD-ARCHITECTURES.md).

Use this repository when you want the ready-to-build llama.cpp fork with
ROCmFP4, MTP, ROCm/HIP, and Vulkan integration already applied.

Canonical continuation repository (private; authenticate with an authorized
GitHub account):

```bash
git clone https://github.com/JCFrags/HaloFPX.git
cd HaloFPX
git checkout main
```

The historical `charlie12345/rocmfp4-llama` donor is provenance, not the
continuation authority. Do not make the combined repository or evidence release
public until the privacy and third-party redistribution gates in
[`LICENSES_AND_PROVENANCE.md`](../LICENSES_AND_PROVENANCE.md) are satisfied.

## Target hardware and operating system

The current physical target is:

```text
nimo-1 and nimo-2
Nimo Direct MME3L
AMD Ryzen AI MAX+ 395 / Radeon 8060S / gfx1151
about 124.45 GiB host-visible memory per node
CachyOS rolling, kernel 7.1.3-1-cachyos
ROCm 7.2.4 family, Mesa/RADV 26.1.4
```

Other Strix Halo systems, Ubuntu, and Framework hardware may work, but they are
not the proof target for a promoted HaloFPX performance claim unless the claim
explicitly names that environment.

## P0 target-ownership gate

Before any build, quantization, disposable inference, or benchmark on either
target, apply [issue #41](https://github.com/JCFrags/HaloFPX/issues/41). Reject
the work while a protected production service or any unaccounted KFD, render,
or HMM owner is active. `MemAvailable`, free RAM, conventional process RSS,
and swap are not sufficient admission predicates: a 2026-08-12 nimo-2 build
window showed about 14 GiB available while production held about 114 GiB of
`gpu_active` HMM pages, followed by global OOM and both-rank service restarts.

Target work requires an authorized maintenance window, exact before-state
service identities, an empty foreign GPU-owner census, and a clean kernel-OOM
baseline. If either rank changes PID, InvocationID, or restart count, health
alone is insufficient: recapture both identities and complete a real minimal
two-rank inference before declaring recovery. The immutable
[incident record](halofpx/evidence/2026-08-12-target-hmm-oom-incident/README.md)
is safety evidence, not a benchmark.

That receipt's latest retained recovery baseline is nimo-1 PID `3113343`,
InvocationID `0656332b63a140eab7214627baa43253`, `NRestarts=1`, and nimo-2
PID `2248760`, InvocationID `d15fe49610274e77bd9a3d84a0b791a5`,
`NRestarts=1`. Both units were active/running, coordinator health was OK, and
a real 5-prompt-token plus 1-generated-token request completed. Recheck these
volatile identities before target work. The incident capture did not rehash
the service executables or loaded libraries.

## Prerequisites

On the existing targets, verify the installed CachyOS toolchain before adding
or upgrading packages:

```bash
uname -a
cat /etc/os-release
pacman -Q cmake ninja gcc clang pkgconf shaderc vulkan-tools \
  rocm-core rocm-hip-runtime hip-runtime-amd rocminfo
/opt/rocm/lib/llvm/bin/clang++ --version
```

The exact fresh-CachyOS bootstrap package set remains a qualification task; do
not mutate production merely to make this guide look complete. A disposable
or approved fresh host may install the normal Arch/CachyOS build tools with
`pacman`, but retain the resolved package versions in the build receipt.

For an Ubuntu portability/control host only, the inherited package command is:

```bash
sudo apt-get update
sudo apt-get install -y git cmake ninja-build build-essential clang pkg-config \
  glslc vulkan-tools
```

ROCm must see the Strix Halo GPU:

```bash
HSA_OVERRIDE_GFX_VERSION=11.5.1 rocminfo
```

Vulkan should list a GPU device:

```bash
vulkaninfo --summary
```

## Build

For a distributed target A/B, prefer the evidence-capturing matched build on
each node independently:

```bash
scripts/build-halofpx-primary-matched.sh \
  "$PWD" /absolute/disposable/build /absolute/restricted/evidence OFF
```

Do not copy one node's binary to the other without proving runtime-library
resolution; an absolute target RUNPATH has failed previously. Hash both
coordinator and worker binaries for every condition.

For the broader interactive tools and quantizer:

```bash
env JOBS=16 scripts/build-strix-rocmfp4-mtp.sh
```

For RDNA2, RDNA3, or RDNA4 desktop GPUs, see
[`BUILD-AMD-ARCHITECTURES.md`](BUILD-AMD-ARCHITECTURES.md) and run the matching
wrapper script (`build-rdna2.sh`, `build-rdna3.sh`, or `build-rdna4.sh`).

The build script enables ROCm/HIP and Vulkan, disables NVIDIA CUDA, targets
`gfx1151`, and writes binaries under:

```text
build-strix-rocmfp4/bin/
```

Key binaries:

```text
llama-cli
llama-server
llama-quantize
llama-bench
test-backend-ops
test-quantize-fns
test-quantize-perf
```

## Quantize A Model

For real quality testing, start from an F16 or BF16 GGUF source. Requantizing an
already heavily quantized file is only useful for smoke tests.

Compact Strix profile:

```bash
./build-strix-rocmfp4/bin/llama-quantize \
  /path/to/source-bf16.gguf \
  /path/to/model-ROCmFP4-STRIX_LEAN.gguf \
  Q4_0_ROCMFP4_STRIX_LEAN
```

Quality-biased Strix profile:

```bash
./build-strix-rocmfp4/bin/llama-quantize \
  /path/to/source-bf16.gguf \
  /path/to/model-ROCmFP4-STRIX.gguf \
  Q4_0_ROCMFP4_STRIX
```

Pure experimental formats:

```bash
./build-strix-rocmfp4/bin/llama-quantize source.gguf out-dual.gguf Q4_0_ROCMFP4
./build-strix-rocmfp4/bin/llama-quantize source.gguf out-fast.gguf Q4_0_ROCMFP4_FAST
```

## Run Interactive ROCm

Use `--jinja` when a model has a modern chat template. Use `--reasoning on` only
for models whose template supports reasoning.

```bash
HSA_OVERRIDE_GFX_VERSION=11.5.1 \
GGML_HIP_ENABLE_UNIFIED_MEMORY=1 \
./build-strix-rocmfp4/bin/llama-cli \
  -m /path/to/model-ROCmFP4-STRIX_LEAN.gguf \
  -dev ROCm0 \
  -ngl 999 \
  -c 262144 \
  -b 512 \
  -ub 512 \
  -fa on \
  -ctk q8_0 \
  -ctv q8_0 \
  --jinja \
  -if
```

## Run Interactive MTP

For a model with native MTP draft heads, add speculative draft flags:

```bash
HSA_OVERRIDE_GFX_VERSION=11.5.1 \
GGML_HIP_ENABLE_UNIFIED_MEMORY=1 \
./build-strix-rocmfp4/bin/llama-cli \
  -m /path/to/model-ROCmFP4-STRIX_LEAN.gguf \
  -dev ROCm0 \
  -ngl 999 \
  -c 262144 \
  -b 512 \
  -ub 512 \
  -fa on \
  -ctk q8_0 \
  -ctv q8_0 \
  --spec-type draft-mtp \
  --spec-draft-n-max 4 \
  --spec-draft-n-min 0 \
  --spec-draft-p-min 0.0 \
  --spec-draft-p-split 0.10 \
  --spec-draft-type-k q4_0 \
  --spec-draft-type-v q4_0 \
  --jinja \
  -if
```

Remove the `--spec-*` flags for models that do not support MTP.

## Validate

After building, run the full promoted gate:

```bash
env HSA_OVERRIDE_GFX_VERSION=11.5.1 scripts/check-rocmfp4-all-regression.sh
```

Focused checks while iterating:

```bash
scripts/check-rocmfp4-quant-regression.sh
scripts/check-rocmfp4-rocm-runtime-regression.sh
scripts/check-rocmfp4-rocm-fattn-regression.sh
scripts/check-rocmfp4-vulkan-runtime-regression.sh
scripts/check-rocmfp4-qwen-mtp-regression.sh
```

Some validation scripts use local default model paths. Override `MODEL`,
`ROCMFP4_MODEL`, or `BASELINE_MODEL` if your models live elsewhere.

## Troubleshooting

- `No HIP GPUs are available`: verify ROCm sees the GPU with
  `HSA_OVERRIDE_GFX_VERSION=11.5.1 rocminfo`.
- Chat template runtime error: add `--jinja`.
- Out of memory or very slow context setup: lower `-c`, `-b`, or `-ub`, then
  retest.
- No model weights are included in this repository. Download model files
  separately and follow their licenses.
- llama.cpp stores the HIP backend in paths named `ggml-cuda`; this build sets
  `-DGGML_CUDA=OFF` and uses those files for AMD HIP/ROCm.
