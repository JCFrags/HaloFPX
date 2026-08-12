# Container recipes

These images pin source commits and major userspace versions. They do not include or replace the host kernel, amdgpu driver, firmware, BIOS, IOMMU policy, or device permissions.

## Files

- `Dockerfile.llama-rocm721` — conservative ROCm 7.2.1 + llama.cpp b10064 HIP build.
- `Dockerfile.rocm714-therock` — ROCm 7.14 gfx1151 tarball + b10064 build candidate. The build deliberately requires an independently recorded tarball checksum.
- `Containerfile.llama-vulkan` — Fedora 43 RADV runtime + b10064 Vulkan build.
- `Dockerfile.rocmfpx` — experimental ROCmFPX a5605 + ROCm 7.2.1.
- `compose.yaml` — local orchestration and device mappings.

## Host preflight

```bash
../scripts/verify-host.sh
export RENDER_GID="$(getent group render | cut -d: -f3)"
export VIDEO_GID="$(getent group video | cut -d: -f3)"
export MODEL_DIR="$PWD/../models"
```

## Build and run

```bash
docker compose -f compose.yaml build llama-rocm721 llama-vulkan rocmfpx
docker compose -f compose.yaml run --rm llama-rocm721
docker compose -f compose.yaml run --rm llama-vulkan
```

ROCm 7.14 additionally requires:

```bash
export ROCM_714_TARBALL_SHA256='independently-recorded-sha256'
docker compose -f compose.yaml build llama-rocm714
```

## Immutable image lock

Run `../scripts/lock-container-digests.sh`, save the output, and replace mutable base tags with `@sha256:` digests. Rebuild after every kernel/firmware change because the host layer is part of the effective profile.

## Security

`seccomp=unconfined` is included for compatibility with common ROCm community recipes. It broadens container permissions. Derive a narrower profile before production use. Never expose the experimental USB4 verbs driver to an untrusted host or cable.
