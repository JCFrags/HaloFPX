# Reproducibility and provenance

## Pins in this snapshot

```text
Wiki version: 2026.07.17
llama.cpp: b10064 / 86d86ed4396b4130922f7b9af26e3d9fc11a591b
llama.cpp ROCm 7.2 asset SHA256: 42a00452f42b04598d32db66c5249b3e8855cd99bf9448e22fd2a738aaa89c82
Mesa: 26.1.5 / SHA256 79e421c7ce18cd9e790b8375920325779f10798630bf30e0b22f1a21c8617122
ROCmFPX: a5605a72768c6562241b248e268e33dc92787394
thunderbolt-ibverbs: 76ba39b630a70accb72f19388eefe48844b50eb8
```

## Reproducibility levels

| Level | Requirement |
|---|---|
| Source-pinned | Exact Git commit or release tarball and checksum |
| Toolchain-pinned | Exact ROCm/HIP/LLVM, CMake/Meson, compiler, and base OS |
| Container-pinned | OCI image digest, not only a mutable tag |
| Host-pinned | Kernel build, firmware package, firmware file hashes, BIOS version, kernel command line |
| Workload-pinned | Model hash, prompt, context, batch, K/V cache, flags, power mode |

The included Dockerfiles use versioned base tags because OCI digests were not captured in the source set. Before production use, resolve and record each digest:

```bash
docker buildx imagetools inspect rocm/dev-ubuntu-24.04:7.2.1-complete
podman image inspect IMAGE --format '{.Digest}'
```

Then replace `FROM image:tag` with `FROM image@sha256:...` in a local lock branch.

## Build metadata capture

After every build:

```bash
cmake -LAH -N build-hip > build-hip/cmake-cache-options.txt
cmake --build build-hip --verbose > build-hip/build.log 2>&1
ldd build-hip/bin/llama-cli | sort > build-hip/llama-cli.ldd.txt
sha256sum build-hip/bin/* > build-hip/SHA256SUMS
```

Capture source state:

```bash
git rev-parse HEAD
git status --short
git submodule status --recursive
```

## ROCm 7.14 tarball limitation

The official versioned gfx1151 tarball URL is pinned in this wiki. The captured AMD page did not provide a tarball checksum in the research record, so [`install-rocm-714-tarball.sh`](../scripts/install-rocm-714-tarball.sh) requires either an operator-supplied `ROCM_TARBALL_SHA256` or explicit `ALLOW_UNVERIFIED=1`. The latter produces a checksum for the build record but is not byte-verification against an independent source.

## Rebuild versus prebuilt

- Use the upstream b10064 ROCm 7.2 binary and its published checksum for the fastest byte-pinned path.
- Use source builds when targeting ROCm 7.14, when changing backend flags, or when producing both HIP and Vulkan in one binary.
- Use ROCmFPX only from its exact commit and container baseline.
- Keep host drivers outside the container model: containers do not replace the host kernel, amdgpu driver, firmware, or device permissions.

## Offline validation included in this release

```bash
make validate
```

The target runs:

- JSON/YAML/CSV consistency checks;
- source-ID integrity checks;
- Python compilation;
- `bash -n` on shell scripts;
- local Markdown/HTML link checks;
- offline site rendering.

No physical GPU execution is implied.
