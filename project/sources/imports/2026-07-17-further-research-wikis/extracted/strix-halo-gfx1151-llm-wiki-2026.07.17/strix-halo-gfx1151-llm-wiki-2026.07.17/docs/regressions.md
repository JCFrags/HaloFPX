# Known regressions and failure modes

Machine-readable records: [`regressions-2026.07.17.json`](../data/regressions-2026.07.17.json), CSV, and YAML.

| ID | Severity | Component | Affected | Symptoms | Mitigation | Evidence |
| --- | --- | --- | --- | --- | --- | --- |
| REG-FW-MES083-20251125 | critical | amdgpu firmware / linux-firmware | linux-firmware 20251125 / MES 0x83 era on Strix Halo | GPU hang, gfxhub page fault, memory access fault, llama-bench segfault, blocked management processes | Upgrade to distribution-fixed firmware (Fedora 20260110 line or newer validated package); do not treat a kernel update alone as sufficient | [ROCM-ISSUE-5724](sources.md#rocm-issue-5724), [FEDORA-BUG-2420062](sources.md#fedora-bug-2420062), [LINUX-FW-REVERT-C092](sources.md#linux-fw-revert-c092) |
| REG-CWSR-MES080-ROCM71 | high | amdgpu CWSR / MES / ROCm 7.1 | ROCm 7.1.x with MES 0x80-era stack | MES REMOVE_QUEUE hang and GPU hang | Move to a currently supported ROCm line; amdgpu.cwsr_enable=0 is only a legacy containment measure | [ROCM-ISSUE-5590](sources.md#rocm-issue-5590), [AMD-RDNA35](sources.md#amd-rdna35) |
| REG-KERNEL-RDNA35-FIXSET | critical | Linux amdgpu kernel driver | Kernels below AMD’s distro-specific thresholds | Compute instability, queue faults, incomplete memory behavior | Use Ubuntu HWE >=6.17.0-19.19~24.04.2, OEM >=6.14.0-1018, or >=6.18.4 elsewhere | [AMD-RDNA35](sources.md#amd-rdna35), [ROCM-ISSUE-5824](sources.md#rocm-issue-5824) |
| REG-LLAMA-ROCWMMA-FA | medium | llama.cpp HIP rocWMMA FlashAttention | gfx1151 long-context prefill on reported ROCm 7.2.1 stack | Up to approximately 41% lower prefill performance | Keep GGML_HIP_ROCWMMA_FATTN=OFF unless local benchmark proves benefit | [LLAMA-ISSUE-24437](sources.md#llama-issue-24437), [LLAMA-GGML-CMAKE-86D86ED](sources.md#llama-ggml-cmake-86d86ed), [LLAMA-ROCM-DOCKER-86D86ED](sources.md#llama-rocm-docker-86d86ed) |
| REG-LLAMA-RADV-PP-B8460-B8933 | medium | llama.cpp Vulkan/RADV prompt processing | Reported transition b8460 to b8933 on Mesa 26.0.2 and Linux 6.19.4 | pp512 decreased by about 39%; token generation stayed approximately stable | Pin known-good commits and gate upgrades with benchmark thresholds | [LLAMA-ISSUE-22375](sources.md#llama-issue-22375) |
| REG-VULKAN-DIRECT-IO-LOAD | medium | llama.cpp Vulkan model loading | Selected filesystems/models/drivers | Model fails during load or mapping | Use --no-direct-io and/or --no-mmap; collect loader and kernel logs | [LLAMA-ISSUE-18741](sources.md#llama-issue-18741), [KYUZ0-TOOLBOX-A7C71E9](sources.md#kyuz0-toolbox-a7c71e9) |
| REG-AMDVLK-2G-ALLOCATION | high-for-large-models | AMDVLK Vulkan memory allocation | Large GGUF buffers exceeding reported per-allocation behavior | Model load fails although sufficient aggregate memory exists | Use Mesa RADV for large models | [LLAMA-ISSUE-15054](sources.md#llama-issue-15054), [KYUZ0-TOOLBOX-A7C71E9](sources.md#kyuz0-toolbox-a7c71e9) |
| REG-HIST-15G-VISIBLE-MEM | historical-high | Kernel/ROCm unified-memory visibility | Kernel <=6.15 and older ROCm-era combinations | Only approximately 15.5 GiB available despite much larger system RAM | Use current fixed kernel/ROCm and official TTM sizing | [ROCM-ISSUE-5444](sources.md#rocm-issue-5444), [AMD-RDNA35](sources.md#amd-rdna35) |
| REG-HIP-POST-FAULT-DSTATE | high | HIP/amdgpu post-fault recovery | Selected fault paths | rocm-smi or workload process blocked in D state | Capture diagnostics before reboot; update kernel/firmware/ROCm as one tested profile | [ROCM-ISSUE-5745](sources.md#rocm-issue-5745) |

## Triage order

When a GPU workload fails, do not start by changing application flags. Establish the lower layers first:

1. **Kernel threshold** — compare `uname -r` with the AMD RDNA 3.5 rule.
2. **Firmware package and files** — reject the 20251125/MES 0x83 era and capture hashes.
3. **ROCm release integrity** — one SDK root, one HIP compiler, one runtime generation.
4. **Minimal HIP smoke** — compile and run [`smoke-hip.cpp`](../scripts/smoke-hip.cpp).
5. **Vulkan loader** — list ICDs and run `vulkaninfo --summary` with loader debug when needed.
6. **llama.cpp A/B** — backend, mmap/direct-I/O, graphs, rocWMMA, and commit pin.
7. **Model/workload** — only after the platform passes minimal tests.

## Fault-signature commands

```bash
sudo dmesg -T | grep -Eai 'amdgpu|MES|gfxhub|page fault|GPU reset|ring timeout|firmware'
ps -eo pid,stat,wchan:32,comm,args | awk '$2 ~ /D/'
cat /proc/cmdline
find /lib/firmware/amdgpu -maxdepth 1 -type f -printf '%f
' | sort
rocminfo
hipconfig --full
```

If `rocm-smi` or another management command blocks after a fault, capture kernel logs and task states before rebooting. The post-fault D-state record exists specifically because repeated tool invocations can obscure the first failure.

## Regression gates for upgrades

Record these values before changing kernel, firmware, ROCm, Mesa, or llama.cpp:

```text
pp512, pp2048, tg128
model load wall time
peak resident memory and reported GPU pool size
new dmesg warnings/errors
Vulkan device/driver ID
HIP runtime and compiler version
```

Fail the upgrade if median performance regresses beyond the chosen tolerance or any new amdgpu fault appears. A default tolerance of 5% for stable local runs is reasonable; noisy thermal systems require tighter environmental control rather than a wider tolerance.
