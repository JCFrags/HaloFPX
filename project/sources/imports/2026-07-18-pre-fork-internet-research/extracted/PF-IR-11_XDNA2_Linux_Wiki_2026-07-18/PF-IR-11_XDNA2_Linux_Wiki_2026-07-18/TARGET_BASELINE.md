# Target baseline

## Hardware identity

[INFERENCE] The expected target is AMD Ryzen AI MAX+ 395, Strix Halo, with an XDNA2 NPU that maps to the upstream `npu5` entry at PCI vendor/device `1022:17f0`, revision `0x11`.

[UPSTREAM] The pinned Linux source maps `0x17f0, 0x11` to `dev_npu5_info`, uses firmware directory `amdnpu/17f0_11/`, reports default VBNV `RyzenAI-npu5`, and exposes a kernel-mode queue device type.

## Target Linux state

[MISSING] The actual target distribution, release, kernel, kernel configuration, `linux-firmware` package, firmware links, `amdxdna` module source, XRT/plugin packages, Ryzen AI package, IOMMU state, and `/dev/accel` permissions were not supplied and were not probed.

[VENDOR-ONLY] AMD's captured Ryzen AI Software 1.7.1 Linux reference environment is Ubuntu 24.04 LTS, kernel 6.10 or newer, Python 3.12.x, and a matched set of versioned XRT, NPU, plugin, and Ryzen AI packages.

[TARGET-DISTRO] The AMD reference environment is not silently treated as the target. Run [`probe/xdna2_readonly_probe.sh`](probe/xdna2_readonly_probe.sh) on the real host before any install or workload test.

## Non-assumptions

[UNSUPPORTED] No `llama.cpp` transformer offload to the XDNA2 NPU is assumed.

[UPSTREAM] No coherent shared-memory behavior is assumed. The upstream driver explicitly describes the NPU as non-cache-coherent and exposes explicit buffer synchronization.

[UNKNOWN] No useful end-to-end performance, energy efficiency, recovery behavior, or operator placement is assumed.
