# Component dossier

The machine-readable table is in [components.csv](../manifests/components.csv) and [components.yaml](../manifests/components.yaml).

## Focus components

### HIP / HSA / KFD

HIP is the user-facing runtime/language layer. ROCR implements the HSA runtime and communicates through KFD in the inbox `amdgpu` kernel driver. Treat `libamdhip64`, `libhsa-runtime64`, COMGR, the device libraries and KFD as one compatibility chain. A successful `rocminfo` enumeration is necessary but not sufficient; queue creation, CWSR stress and kernel dispatch must also pass.

### rocBLAS and hipBLASLt

`hipBLAS` is the common API layer; `rocBLAS` and `hipBLASLt` are distinct implementation paths. In Core SDK 7.14 they are consolidated into `amdrocm-blas`. The 7.14 performance workaround for affected Ryzen AI MAX LLM inference forces the hipBLASLt backend. Record `TORCH_BLAS_PREFER_HIPBLASLT` and any tuning/kernel cache identity in the runtime manifest.

### rocWMMA

rocWMMA is 2.2.0 in the 7.2.4 lane and 2.2.1 in Core SDK 7.14.0. Its headers compile through the lane's HIP/LLVM toolchain and target gfx1151. The transition table associates legacy `rocwmma` with the consolidated `amdrocm-ccl` migration row; verify the installed package file owner rather than inferring from the row label.

### LLVM and compiler resources

Do not hardcode a versioned Clang resource directory. Capture it from the installed compiler:

```bash
amdclang --print-resource-dir
clang --print-resource-dir
hipconfig --path
hipconfig -R
hipconfig -l
find "$ROCM_PATH" -path '*/amdgcn/bitcode' -type d -print
find "$ROCM_PATH" -path '*/lib/clang/*' -type d -print
```

For the Core SDK tarball, the root contains `bin` and `lib`; the exact versioned resource directory and device-bitcode location must be hashed after extraction. For native Core SDK packages, resolve `/opt/rocm` compatibility symlinks back to `/opt/rocm/core` package owners.

### RCCL packaging

[LANE_BOUNDARY] `amdrocm-ccl` is not RCCL. It consolidates rocPRIM, rocThrust and hipCUB. RCCL is packaged separately:

| Package | Role | Relevant dependencies from package metadata | GFX-specific package |
|---|---|---|---|
| `amdrocm-rccl` | runtime/lib/doc | base, AMD SMI, profiler base, libc on Debian | no (`Gfxarch: false`) |
| `amdrocm-rccl-devel` | headers/development | `amdrocm-rccl`, Python/system runtime | no |
| `amdrocm-rccl-test` | tests | base, profiler base, rccl-devel | no |

The architecture-specific Core SDK meta package supplies the compatible GPU kernel packs and math/runtime dependencies around this host-side RCCL package.

## Dependency boundary

See [dependencies.dot](../manifests/dependencies.dot). Exact ELF linkage, RPATH and package ownership are acquisition-time facts and are collected by `scripts/inventory-artifact.sh` and `scripts/capture-runtime-tuple.sh`.

## Version comparison

| Component | ROCm 7.2.4 lane | Core SDK 7.14.0 | 7.14 package grouping | Notes |
|---|---:|---:|---|---|
| Composable Kernel | 1.2.0 | 1.2.0 | amdrocm-ck | math/ML kernels |
| hipBLAS | 3.2.0 | 3.5.0 | amdrocm-blas | BLAS API front end |
| hipBLASLt | 1.2.2 | 1.4.1 | amdrocm-blas | GEMM/Lt back end; performance-sensitive |
| hipCUB | 4.2.0 | 4.5.0 | amdrocm-ccl | device primitives; not RCCL |
| hipFFT | 1.0.22 | 1.0.24 | amdrocm-fft | FFT front end |
| hipRAND | 3.1.0 | 3.4.0 | amdrocm-rand | random API front end |
| hipSOLVER | 3.2.0 | 3.5.0 | amdrocm-solver | solver front end |
| hipSPARSE | 4.2.0 | 4.6.0 | amdrocm-sparse | sparse front end |
| hipSPARSELt | 0.2.6 | 0.2.9 | amdrocm-blas | 7.14 release table limits to Instinct; do not assume gfx1151 |
| MIOpen | 3.5.1 | 3.5.2 | amdrocm-dnn | deep learning primitives |
| rocBLAS | 5.2.0 | 5.5.0 | amdrocm-blas | BLAS implementation |
| rocFFT | 1.0.36 | 1.0.38 | amdrocm-fft | FFT implementation |
| rocPRIM | 4.2.0 | 4.5.0 | amdrocm-ccl | device primitives; not RCCL |
| rocRAND | 4.2.0 | 4.5.0 | amdrocm-rand | random implementation |
| rocSOLVER | 3.32.0 | 3.35.0 | amdrocm-solver | solver implementation |
| rocSPARSE | 4.2.0 | 4.7.0 | amdrocm-sparse | sparse implementation |
| rocThrust | 4.2.0 | 4.5.0 | amdrocm-ccl | parallel algorithms; not RCCL |
| rocWMMA | 2.2.0 | 2.2.1 | amdrocm-ccl (legacy mapping includes rocwmma; verify installed package file owner) | wave matrix templates |
| RCCL | 2.27.7 | 2.30.4 | amdrocm-rccl | collective communication |
| rocSHMEM | 3.2.0 | 3.5.0 | amdrocm-rocshmem | GPU-centric SHMEM |
| rocDecode | 1.7.0 | 1.8.0 | amdrocm-decode | media decode; gfx1151 listed in 7.14 applicability |
| rocJPEG | 1.4.0 | 1.6.0 | amdrocm-jpeg | JPEG |
| hipFile | not listed | 0.3.0 | see package manifest | 7.14 release table limits to Instinct |
| HIP | 7.2.1 | 7.14 | amdrocm-runtime | runtime and language |
| ROCr Runtime / HSA | 1.18.0 | 1.21.0 | amdrocm-runtime | HSA runtime |
| LLVM | 22.0.0 | 23.0.0 | amdrocm-llvm | compiler/toolchain |
| HIPIFY | 22.0.0 | 7.14 | amdrocm-hipify | source translation |
| SPIRV-LLVM-Translator | not listed | 23.0.0 | amdrocm-llvm/runtime dependency | SPIR-V translation |
| rocminfo | package-version capture required | 1.0.0 | amdrocm-base | runtime discovery |
| AMD SMI | legacy ROCm SMI/AMD SMI capture required | 26.5.0 | amdrocm-amdsmi | monitoring; APU metrics added |
| ROCm Compute Profiler | capture required | 3.7.0 | amdrocm-profiler | gfx1151 known counter issues |
| ROCprofiler-SDK | capture required | 1.3.2 | amdrocm-profiler / profiler-base | profiling runtime |
