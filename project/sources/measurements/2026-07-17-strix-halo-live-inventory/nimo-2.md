# nimo-2 snapshot

Capture window: 2026-07-17 11:52–12:05 PDT.

## Platform

- Nimo Direct MME3L / NIMO Mini PC, board version 1.0.
- AMI BIOS 3.05 dated 2025-10-11.
- AMD Ryzen AI MAX+ 395, 16 cores / 32 threads, stepping 0, microcode `0xb700037`.
- Radeon 8060S / `gfx1151`, PCI `1002:1586` revision `c1`, 40 compute units.
- 130,491,700 KiB physical memory reported; one NUMA node.
- CachyOS, active kernel `7.1.3-1-cachyos`.

## Memory and accelerator policy

- Kernel command line matches nimo-1's GTT, TTM, IOMMU, C-state, and ASPM settings and additionally contains `zswap.enabled=0`.
- `rocminfo` enumerated the CPU and `gfx1151`; `/dev/kfd` and `/dev/dri/renderD128` existed with mode `0666`.
- `rocm-smi` reported 133,143,986,176 bytes total GTT and approximately 265 MB used during the capture.
- 32 GiB swapfile; capture-time use was approximately 456 MiB, priority `100`.

## Storage

- One Crucial P310 `CT1000P310SSD8`, nominal 1 TB, firmware `VACR001`, Btrfs root/home/cache layout.
- Filesystem size 999,665,881,088 bytes; 658,246,303,744 bytes used; estimated free 341,008,699,392 bytes (about 318 GiB).
- SMART: critical warning `0`, spare 100%, percentage used 0%, 5.51 TB written, 2,095 power-on hours, 17 unsafe shutdowns, zero media/data-integrity errors, zero error-log entries, 30 C.
- Major visible consumers: `/opt/llm-usb4-cluster` about 171 GiB, including models about 166 GiB; `/home/connorb/ds4-models` about 154 GiB; user cache about 15 GiB.

## Fabric

- LAN: `eno1`, Realtek RTL8125, 2.5 Gb/s, `192.168.40.12/24`.
- Rail A: `thunderbolt0`, `10.44.0.2/30`, maps to USB4 domain 1 / PCI function `c7:00.6`.
- Rail B: `thunderbolt1`, `10.44.0.6/30`, maps to USB4 domain 0 / PCI function `c7:00.5`.
- Each remote USB4 path reported two RX and two TX lanes at 20.0 Gb/s per lane; MTU 9000.
- MPTCP enabled. The secondary endpoint was `10.44.0.6 id 3 signal subflow`; limits were two accepted addresses and two subflows.
- The active client MPTCP socket contained two subflows, one on each private rail.
- Five-packet diagnostic RTT: rail A average 0.097 ms; rail B average 0.087 ms; zero loss.

## Active runtime

- `minimax-m27-rocmfp4-dual-server.service` was active since 2026-07-15 20:56 PDT.
- `llama-server` used remote `RPC0` followed by local `ROCm0`, layer split `1,1`, two 4096-token slots inside `--ctx-size 8192`, Q4_0 K/V cache, batch 4096, ubatch 512, `--fit off`, `--no-mmap`, and `--no-warmup`.
- RSS approximately 64.4 GiB; cgroup memory approximately 79.6 GiB at capture.
- Executable SHA-256: `ab9c0275289857811154e17fdffd35bb857ce20a1b0fdcf00e3c85e82de5a479`.
- Deployed source checkout: `charlie12345/rocmfp4-llama` commit `4860505ee322091f0f61eba77d6ad49be88cf4ea`, detached and clean.
- Active artifact: `/opt/llm-usb4-cluster/models/rcmorano_llmfan46-minimax-m2.7-ultra-uncensored-heretic-rocmfp/f4e1087425e02cd770d29a9ece5912e5e0730f41/llmfan46-MiniMax-M2.7-ultra-uncensored-heretic-ROCMFP4.gguf`.
- Active GGUF size was 121,861,632,736 bytes. The model API reported 228,689,764,864 parameters and model payload size 121,853,344,768 bytes.
- Loopback `/health` returned `{"status":"ok"}` and `/v1/models` returned the configured model. These are readiness observations, not correctness or performance results.
- API listened on `0.0.0.0:8082`; SSH and LLMNR were the other externally bound TCP listeners observed.
- No failed systemd units were reported in the initial capture.

## Software snapshot

- ROCm packages 7.2.4; HIP version string began `7.2.53211-3d9ef42` in captured output.
- Mesa 26.1.4, linux-firmware 20260622, GCC 16.1.1, Clang 22.1.6, CMake 4.3.4, Ninja 1.13.2, Python 3.14.6.
- Installed LTS kernel packages were one point behind nimo-1 (`6.18.37` versus `6.18.38`), although both machines were running the same `7.1.3-1-cachyos` kernel.
- Kernel config had `CONFIG_USB4=m` and `CONFIG_USB4_NET=m`; no `thunderbolt_stream` module or `/dev/tbstream*` device existed.

## Point-in-time idle telemetry

- GPU use 0%; edge 41 C; reported package power about 15 W.
- CPU Tctl 42.4 C; NVMe 30 C.
- These values were captured while the model remained loaded but no inference was submitted.
