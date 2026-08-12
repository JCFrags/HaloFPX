# nimo-1 snapshot

Capture window: 2026-07-17 11:52–12:05 PDT.

## Platform

- Nimo Direct MME3L / NIMO Mini PC, board version 1.0.
- AMI BIOS 3.05 dated 2025-10-11.
- AMD Ryzen AI MAX+ 395, 16 cores / 32 threads, stepping 0, microcode `0xb700037`.
- Radeon 8060S / `gfx1151`, PCI `1002:1586` revision `c1`, 40 compute units.
- 130,491,708 KiB physical memory reported; one NUMA node.
- CachyOS, active kernel `7.1.3-1-cachyos`.

## Memory and accelerator policy

- Kernel command line includes `amdgpu.gttsize=126976`, `ttm.pages_limit=32505856`, `ttm.page_pool_size=32505856`, `amd_iommu=off`, `processor.max_cstate=2`, and `pcie_aspm=off`.
- `rocminfo` enumerated the CPU and the `gfx1151` GPU; `/dev/kfd` and `/dev/dri/renderD128` existed with mode `0666`.
- `rocm-smi` reported 133,143,986,176 bytes total GTT and approximately 268 MB used during the capture. This is not total process-resident model memory.
- 32 GiB swapfile; capture-time use was approximately 18 MB, priority `-1`.

## Storage

- One Crucial P310 `CT1000P310SSD8`, nominal 1 TB, firmware `VACR001`, Btrfs root/home/cache layout.
- Filesystem size 999,665,881,088 bytes; 952,040,550,400 bytes used; estimated free 46,143,823,872 bytes (about 43 GiB).
- SMART: critical warning `0`, spare 100%, percentage used 1%, 6.10 TB written, 2,210 power-on hours, 14 unsafe shutdowns, zero media/data-integrity errors, zero error-log entries, 32 C.
- Major visible consumers: `/opt/llm-usb4-cluster` about 392 GiB, including models about 276 GiB and RPC tensor cache about 112 GiB; `/home/connorb/ds4-models` about 157 GiB; user cache about 49 GiB.

## Fabric

- LAN: `eno1`, Realtek RTL8125, 2.5 Gb/s, `192.168.40.11/24`.
- Rail A: `thunderbolt0`, `10.44.0.1/30`, maps to USB4 domain 0 / PCI function `c7:00.5`.
- Rail B: `thunderbolt1`, `10.44.0.5/30`, maps to USB4 domain 1 / PCI function `c7:00.6`.
- Each remote USB4 path reported two RX and two TX lanes at 20.0 Gb/s per lane; MTU 9000.
- MPTCP enabled. The secondary endpoint was `10.44.0.5 id 3 signal subflow`; limits were two accepted addresses and two subflows.
- The active RPC MPTCP socket contained two subflows, one on each private rail.
- Five-packet diagnostic RTT: rail A average 0.095 ms; rail B average 0.100 ms; zero loss.

## Active runtime

- `minimax-m27-rocmfp4-rpc-worker.service` was active since 2026-07-15 20:56 PDT.
- Process: `rpc-server --host 10.44.0.1 --port 50052 --device ROCm0 --cache`.
- RSS approximately 58.9 GiB; cgroup memory approximately 76.0 GiB at capture.
- Executable SHA-256: `7f7cb7f0b2217ed714e32d028c210059d78dc932caf2b1a78055d23b59b99d9a`.
- Deployed source checkout: `charlie12345/rocmfp4-llama` commit `4860505ee322091f0f61eba77d6ad49be88cf4ea`, detached and clean.
- RPC listener was bound only to `10.44.0.1:50052`; SSH was the only other externally bound TCP listener observed.
- No failed systemd units were reported in the initial capture.

## Software snapshot

- ROCm packages 7.2.4; HIP version string `7.2.53211-3d9ef427.2.4`.
- Mesa 26.1.4, linux-firmware 20260622, GCC 16.1.1, Clang 22.1.6, CMake 4.3.4, Ninja 1.13.2, Python 3.14.6.
- `hipcub` and the aggregate `rocm-hip-sdk` package were installed here but were not present in the nimo-2 package subset captured.
- Kernel config had `CONFIG_USB4=m` and `CONFIG_USB4_NET=m`; no `thunderbolt_stream` module or `/dev/tbstream*` device existed.

## Point-in-time idle telemetry

- GPU use 0%; edge 43 C; reported package power about 16 W.
- CPU Tctl 44.2 C; NVMe 32 C.
- These values were captured while the model remained loaded but no inference was submitted.

