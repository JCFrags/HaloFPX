# Officially supported combinations

## Support scopes that currently coexist

### ROCm 7.14 Core SDK

AMD’s current matrix lists gfx1151 in the ROCm 7.14 hardware set. The captured supported host combinations are Ubuntu 26.04 with kernel 7.0 GA and Ubuntu 24.04.4 with HWE kernel 6.17, using the inbox kernel driver. ROCm/HIP 7.14 is paired with LLVM 23.0.0. Sources: [AMD-CORE-714](sources.md#amd-core-714).

This is the correct lane for current SDK development. It does **not** by itself state that the Ryzen llama.cpp binary page, PyTorch wheel matrix, or ROCmFPX fork has been validated against 7.14.

### RDNA 3.5 target-specific release table

AMD’s RDNA 3.5 page establishes the current fixed-minimum kernel rules:

```text
Ubuntu 24.04 HWE: 6.17.0-19.19~24.04.2 or newer
Ubuntu 24.04 OEM: 6.14.0-1018 or newer
Other distributions: 6.18.4 or newer
```

On qualifying kernels, the captured prebuilt-release table marks ROCm 7.2.1, 7.2.2, and 7.2.3 stable. ROCm 7.1.x and 6.4.x are not supported in that row. Source: [AMD-RDNA35](sources.md#amd-rdna35).

### Ryzen application-support line

The ROCm 7.2 Radeon/Ryzen documentation includes gfx1151 and provides application-oriented validation, including a production-supported PyTorch 2.9 / ROCm 7.2 / Python 3.12 line. Ubuntu 24.04.3 is documented through a preliminary 24.04.2 installer path. Sources: [AMD-RYZEN-72](sources.md#amd-ryzen-72), [AMD-RYZEN-INDEX](sources.md#amd-ryzen-index).

### General ROCm 7.2.4 release

ROCm 7.2.4 is an official release with HIP 7.2.53211, LLVM 22.0.0.26084, ROCr 1.18.0, and ROCm CMake 0.14.0. The general matrix is authoritative for those component versions. It is not, by itself, an explicit gfx1151 application-support row. Source: [AMD-CORE-724](sources.md#amd-core-724).

## Official/upstream components outside AMD application certification

| Component | Official statement | What it does not prove |
|---|---|---|
| llama.cpp b10064 | Upstream release and ROCm 7.2 binary asset | Host kernel/firmware correctness or AMD application certification |
| Mesa 26.1.5 | Current upstream Mesa bugfix release | gfx1151 llama.cpp performance or regression-free behavior |
| CMake 4.3.4 | Current Kitware release | Compatibility with every older ROCm package layout |
| `CONFIG_USB4_NET` | Upstream kernel networking component | A particular cable’s speed or distributed LLM scaling |

Sources: [LLAMA-RELEASE-B10064](sources.md#llama-release-b10064), [MESA-2615](sources.md#mesa-2615), [CMAKE-RELEASE-434](sources.md#cmake-release-434), [LINUX-USB4-NET-KCONFIG](sources.md#linux-usb4-net-kconfig).

## Historical official path

AMD’s old ROCm 7.1.1 llama.cpp page documents `llama-b7146-ubuntu-24.04-rocm-7.1.1-gfx1150-gfx1151-x64.zip`. Retain it only for provenance. AMD’s current RDNA 3.5 table now classifies ROCm 7.1.x as unsupported on qualifying kernels, and the CWSR/MES regression record makes it a poor new deployment choice. Sources: [AMD-LLAMA-711](sources.md#amd-llama-711), [AMD-RDNA35](sources.md#amd-rdna35), [ROCM-ISSUE-5590](sources.md#rocm-issue-5590).

## Official profile checklist

- Match the exact supported OS and kernel family.
- Use the supported inbox/distribution kernel driver rather than mixing arbitrary DKMS and firmware packages.
- Keep HIP, LLVM, ROCr, and CMake packages from one ROCm release line.
- Verify `/dev/kfd`, `/dev/dri`, `rocminfo`, and a compiled HIP smoke test before application testing.
- Treat application matrices separately from the Core SDK matrix.
