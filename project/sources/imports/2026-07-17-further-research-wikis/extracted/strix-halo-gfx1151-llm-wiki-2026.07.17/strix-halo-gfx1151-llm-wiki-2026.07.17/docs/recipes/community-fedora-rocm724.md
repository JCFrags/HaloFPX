# Recipe: maintained Fedora / ROCm 7.2.4 community profile

**Classification:** official general ROCm release plus maintained community gfx1151 validation  
**Reference profile:** Fedora 42/43, kernel 6.18.9, linux-firmware 20260110, ROCm 7.2.4  
**Sources:** [KYUZ0-TOOLBOX-A7C71E9](../sources.md#kyuz0-toolbox-a7c71e9), [AMD-CORE-724](../sources.md#amd-core-724), [AMD-RDNA35](../sources.md#amd-rdna35)

## Host checks

```bash
rpm -q kernel-core linux-firmware
uname -r
sudo dmesg -T | grep -Eai 'amdgpu|MES|gfxhub|page fault|firmware' | tail -n 200
```

Reject firmware 20251125 and kernels below 6.18.4. The maintained baseline names 6.18.9 and firmware 20260110; newer packages require a fresh acceptance run.

## Toolbx

```bash
toolbox create llama-rocm-724 \
  --image docker.io/kyuz0/amd-strix-halo-toolboxes:rocm-7.2.4 \
  -- --device /dev/dri --device /dev/kfd \
     --group-add video --group-add render \
     --security-opt seccomp=unconfined

toolbox enter llama-rocm-724
llama-cli --list-devices
```

On Ubuntu or another distribution using Distrobox, use the corresponding `distrobox create` command and preserve the same device mappings.

## Pin the mutable image

```bash
skopeo inspect docker://docker.io/kyuz0/amd-strix-halo-toolboxes:rocm-7.2.4 \
  | tee rocm-7.2.4-image-inspect.json
```

Record the digest and recreate the toolbox from `image@sha256:...`.

## Runtime

```bash
llama-cli --no-mmap -ngl 999 -fa 1 \
  -m MODEL.gguf -p 'community profile smoke' -n 64
```

`--no-mmap` is part of this community recipe, not a universal upstream requirement. Also run a small model without it to distinguish platform health from I/O behavior.

## Unified memory

Use AMD’s `amd-ttm` tool and leave a deliberate OS reserve. The repository’s 128 GiB example uses large GTT/TTM values and reports `amd_iommu=off` performance gains. This wiki does not make IOMMU disablement a default because it weakens DMA isolation and conflicts with conservative USB4/RDMA security.

## Status wording

Use this exact description in deployment records:

> ROCm 7.2.4 is an official general release and this Fedora/toolbox combination is community-validated on Strix Halo. The captured AMD RDNA 3.5 target table explicitly names 7.2.1–7.2.3, not 7.2.4.
