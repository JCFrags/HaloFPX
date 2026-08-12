# Reproducible containers

Container definitions are under [`containers/`](../containers/README.md).

## Images

| File | Purpose | Classification |
|---|---|---|
| `Dockerfile.llama-rocm721` | b10064 built with ROCm 7.2.1 and gfx1151 safe flags | Conservative supported-target lane |
| `Dockerfile.rocm714-therock` | b10064 built from official gfx1151 ROCm 7.14 tarball | Reproducible candidate; app validation separate |
| `Containerfile.llama-vulkan` | b10064 Vulkan build with distro RADV runtime | Upstream/community lane |
| `Dockerfile.rocmfpx` | ROCmFPX a5605 on its ROCm 7.2.1 baseline | Experimental community |
| `compose.yaml` | Device mappings and smoke commands | Local orchestration |

## Host requirements remain outside the image

A container cannot repair:

- a kernel below the gfx1151 fixed minimum;
- bad amdgpu firmware;
- missing `/dev/kfd` or `/dev/dri` permissions;
- an incompatible IOMMU/security policy;
- a USB4 kernel-module issue.

## Typical device mapping

HIP:

```bash
docker run --rm -it   --device=/dev/kfd   --device=/dev/dri   --group-add "$(getent group render | cut -d: -f3)"   --group-add "$(getent group video | cut -d: -f3)"   --security-opt seccomp=unconfined   -v "$PWD/models:/models:ro"   IMAGE llama-cli --list-devices
```

Vulkan:

```bash
docker run --rm -it   --device=/dev/dri   --group-add "$(getent group render | cut -d: -f3)"   -v "$PWD/models:/models:ro"   IMAGE llama-cli --list-devices
```

The `seccomp=unconfined` setting is common in community examples but broadens process privileges. Prefer a tailored seccomp profile after identifying required syscalls.

## Digest locking

Version tags are not immutable. Resolve each base image digest and pin it before treating an image as reproducible. The included [`lock-container-digests.sh`](../scripts/lock-container-digests.sh) prints digest metadata when Docker or Podman is available.
