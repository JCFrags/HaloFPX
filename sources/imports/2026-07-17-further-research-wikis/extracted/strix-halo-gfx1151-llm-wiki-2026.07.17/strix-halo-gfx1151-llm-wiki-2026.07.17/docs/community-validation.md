# Community-reported and community-validated combinations

## Maintained Fedora / toolbox baseline

The current maintained community profile reports:

```text
OS: Fedora 42/43
Kernel: 6.18.9-200.fc43.x86_64
Firmware: linux-firmware 20260110
ROCm: 7.2.4 toolbox
Vulkan: Mesa RADV preferred
```

The repository also offers ROCm 7.2.4, ROCm 6.4.4 with a backported kernel patch, RADV, AMDVLK, and experimental ROCmFP4 images. This wiki promotes only the 7.2.4 and RADV paths as current community baselines; the patched 6.4.4 image remains useful for comparison but conflicts with AMD’s current target-specific support classification. Source: [KYUZ0-TOOLBOX-A7C71E9](sources.md#kyuz0-toolbox-a7c71e9).

### Runtime practices reported by that profile

```bash
llama-cli --no-mmap -ngl 999 -fa 1 -m MODEL.gguf -p 'test'
```

`--no-mmap` is a community stability choice, not an upstream universal requirement. Preserve an A/B path without it so that a filesystem or direct-I/O failure is not misdiagnosed as a compute failure.

The repository also reports a performance profile using:

```text
amd_iommu=off amdgpu.gttsize=126976 ttm.pages_limit=32505856
```

on a 128 GiB system. This is **not** adopted as a default here. Disabling IOMMU reduces DMA isolation and can be inappropriate for USB4, virtualization, or research RDMA. Use AMD’s `amd-ttm` guidance first; benchmark any IOMMU change inside the relevant security model.

## HIP versus Vulkan

A first-hand gfx1151 report using ROCm 7.2.1 and Mesa 26.0.3 measured substantially faster generation with Vulkan than HIP for one Qwen3.6 35B-A3B Q8_0 workload. ROCmFPX’s maintainer also reports Vulkan as the faster tested decode backend on that system. These results justify a **Vulkan-first test**, not a universal verdict. Sources: [LLAMA-ISSUE-24438](sources.md#llama-issue-24438), [ROCMFPX-A5605](sources.md#rocmfpx-a5605).

Benchmark both paths with the same:

- model file and checksum;
- prompt and token counts;
- batch and micro-batch;
- context and K/V cache types;
- FlashAttention setting;
- power mode and temperature;
- llama.cpp commit.

## RADV and AMDVLK

RADV is the maintained community default because it covers more large-model cases. AMDVLK can be faster in selected paths, but a reported roughly 2 GiB single-allocation limitation can prevent large GGUF loads despite sufficient aggregate memory. Sources: [KYUZ0-TOOLBOX-A7C71E9](sources.md#kyuz0-toolbox-a7c71e9), [LLAMA-ISSUE-15054](sources.md#llama-issue-15054).

## Community firmware resolution

Fedora’s regression record identifies the 20260110 firmware package line as fixed relative to the broken 20251125 line. Treat package dates as distribution-specific labels, not universal firmware ABI numbers; capture file hashes and dmesg firmware versions in every bug report. Sources: [FEDORA-BUG-2420062](sources.md#fedora-bug-2420062), [ROCM-ISSUE-5724](sources.md#rocm-issue-5724).

## Community validation acceptance gate

A profile graduates to “locally accepted” only after it passes:

1. 30-minute repeated HIP or Vulkan smoke without new amdgpu faults.
2. `llama-bench` prompt processing and generation with stable variance.
3. Model load both with normal I/O and, when needed, `--no-direct-io --no-mmap`.
4. Suspend/resume or at least one warm reboot when the deployment requires it.
5. USB4 throughput and packet-loss test when distributed execution is in scope.
6. A saved diagnostics bundle from [`collect-diagnostics.sh`](../scripts/collect-diagnostics.sh).
