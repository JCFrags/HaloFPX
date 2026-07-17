---
type: deep-system-audit
status: complete
date: 2026-07-12
host: nimo-2
scope: read-only
sensitivity: internal
---

# nimo-2 deep system audit

## Audit contract

- Target: SSH alias `nimo-2`.
- Collection window: 2026-07-12 00:38-00:48 PDT.
- Method: read-only SSH inspection with noninteractive commands. No remote files, packages, services, settings, repositories, or firmware were changed.
- Local changes: this report plus the project manifest/changelog references required by the workspace contract.
- Redaction: machine IDs, device UUIDs/GUIDs, serial numbers, MAC addresses, SSH key paths, and credential-like values are omitted or replaced with `<redacted>`. Private USB4 addresses are retained because they are required to describe the cluster topology and are already canonical project facts.
- Authority: live state and the 2026-07-12 MiniMax unload receipt are authoritative. Older “loaded” descriptions are historical.
- Review: independently peer-reviewed and approved after exact-command, host-key, GTT-aperture, firewall-risk, and cross-node-asymmetry corrections.

## Executive verdict

`nimo-2` is healthy, idle, and fundamentally suitable for the proposed topology-aware llama.cpp fork. The hardware is not the limiting problem. The decisive constraint is the enormous gap between local UMA bandwidth and the measured USB4NET payload path:

- local LPDDR5-8000 theoretical bandwidth is 256 GB/s (256-bit interface at 8 GT/s);
- both USB4 links negotiate 40 Gb/s each, but prior controlled testing measured only about 10.35 Gb/s of TCP payload per rail and 20.705 Gb/s aggregate, or about 2.59 GB/s across both rails;
- prior two-node RCCL tests measured 64.8-244.6 us for 4 KiB-256 KiB collectives and found that the second rail helped messages at or above 1 MiB but did not improve decode-sized small collectives.

That is roughly a 99:1 local-memory-to-dual-rail-payload bandwidth ratio before application protocol overhead. The proposed execution order is therefore correct:

1. replica/data parallel as the default when a model fits locally;
2. a single contiguous layer boundary for oversized models;
3. cross-node speculative draft/verify for single-stream improvement;
4. no row/tensor parallel as an initial product mode.

The machine is not yet a development-grade measurement platform. It has a pinned HIP+RPC llama.cpp binary runtime, but no retained source/build metadata, no Vulkan llama.cpp build, no ROCm profiler, no Linux `perf`, and no real STREAM/fio/sysstat tool set. The first engineering milestone should be reproducible dual-backend builds plus telemetry, not speculative low-level tuning.

## Ranked findings

| Rank | Finding | Evidence | Consequence / action |
|---:|---|---|---|
| 1 | Replica and one-boundary pipeline modes match the measured topology; tensor parallel does not. | Historical dual-rail TCP: 20.705 Gb/s aggregate. RCCL: 4 KiB 64.81 us, 16 KiB 118.31 us, 256 KiB 244.58 us; dual rail mainly helps at 1 MiB and larger. | Make replica/session-affine routing the production default. Keep a whole layer or whole MoE layer on one node. Measure bytes/token and boundary stalls before adding any tensor split. |
| 2 | The current llama.cpp runtime is not a reproducible fork baseline. | Runtime reports commit `8f114a9b573b69035299f9b924047f53c1e22c7e`, HIP+RPC, gfx1151, rocWMMA FlashAttention. Only copied binaries/libraries and a tarball remain; source, `.git`, `CMakeCache.txt`, and install manifest are absent. | Establish a clean source checkout, build manifest, patch queue, ABI/versioning policy, and separate HIP/Vulkan build trees before modifying RPC or kernels. |
| 3 | USB4NET is leaving most nominal bandwidth unused and has a concentrated CPU datapath. | Each interface has one RX and one TX queue, no RSS, RPS, XPS, rings, or coalescing controls. Kernel warns: “Driver has suspect GRO implementation, TCP performance may be compromised.” Heavy data IRQs resolve to a small set of CPUs. | Profile IRQ/softirq and copy cost. A/B explicit IRQ affinity, RPS, socket thread count, batching, message size, and per-rail assignment. Preserve the low-latency result while improving bulk throughput. |
| 4 | Direct USB4 streaming is not available in the running kernel. | Kernel exposes `CONFIG_USB4=m` and `CONFIG_USB4_NET=m`; there is no thunderbolt-stream/USB4STREAM config, module, class, or device node. | Keep USB4NET as the reliable baseline. Treat direct streaming as a separate kernel/transport prototype with explicit compatibility and rollback requirements. |
| 5 | Performance tuning remains latched after the model was intentionally unloaded. | `step37-performance-tuning.service` is disabled but still active/exited; CPU governor/EPP are `performance`, minimum CPU frequency is 2.0 GHz, GPU is forced `high` at 2.9 GHz, CPU PM-QoS is 100 us, USB4 controllers are forced on, and global busy-poll is 100 us. Idle SoC power was about 18.4 W. | Give each runtime an apply/restore lifecycle. Keep an “idle/balanced” state and a reproducible “benchmark/inference” state; do not let historical one-shot services silently determine new tests. |
| 6 | The 124 GiB GTT aperture/cap can address nearly all usable RAM and the worker has no swap safety net. | MemTotal 130,491,700 kB; configured GTT cap 133,143,986,176 bytes (124 GiB), but only 18,677,760 bytes used while unloaded; `ttm.pages_limit=32505856`; no swap; `amdgpu.no_system_mem_limit=Y`. | The cap is not preallocated RAM. A replicated model must still leave room for runtime, KV, staging, HTTP, page tables, and OS reserve. Add admission control based on measured resident/GTT use; do not use “128 GB” as an allocatable model budget. |
| 7 | `amd_iommu=off` has a functional and security cost. | Zero IOMMU groups; ROCm reports `IOMMU Support: None`; the amdxdna NPU driver fails at boot because it cannot run without IOMMU. Secure Boot is disabled and kernel lockdown is off. | Revisit only as a rebooted A/B test because it may affect large SVM mappings and performance. If kept off, document that the NPU is intentionally sacrificed and USB4/PCIe DMA trust is broader. |
| 8 | Firewall policy is split between UFW and firewalld. | Both are active. UFW defaults to allow incoming and retains a stale `8081/tcp from 10.0.0.0/24` rule; firewalld owns the live interfaces. The `llm-usb4` firewalld zone has target `ACCEPT`, allowing every service from the peer. | Converge on one firewall manager. Restrict the USB4 zone to the required control/data ports once the backend protocol is defined. |
| 9 | The development/profiling stack is too thin for kernel work. | No `perf`, rocprofv3/rocprof-compute, rocgdb, fio, stress-ng, sysstat, nvme-cli, memory STREAM benchmark, bpftrace, or trace-cmd. hipBLASLt and rocWMMA development components are absent. | Install a pinned, documented measurement tool set before optimization. Capture profiler version and permissions with every result. |
| 10 | HIP and Vulkan are both viable, but only HIP is present in llama.cpp. | ROCm 7.2.4 sees gfx1151/40 CUs/wave32 and 126,976 MiB. RADV Mesa 26.1.4 exposes Vulkan 1.4, 32-64 subgroup control, cooperative matrices, FP16/int8, and 8/16-bit storage. No `libggml-vulkan` exists in the retained runtime. | Build both from the same source commit and autotune by model, quantization, context, batch, and kernel shape. |
| 11 | Preserved unit inventory contains stale definitions. | `llama-step37.service` references a missing `/home/connorb/llama.cpp` and missing model paths. `step37-container.service` references a container that is absent. The only preserved container visible is the exited DeepSeek toolbox. | Separate active, reloadable, and historical profiles. Add preflight path/image checks so a stale unit cannot be mistaken for a ready rollback. |
| 12 | Storage and base hardware are healthy; there is no immediate fault remediation. | NVMe SMART passed, 0% used, 0 media/errors, 30 C, Btrfs device counters all zero, no coredumps, no failed units, kernel taint 0, and current load/PSI are effectively idle. | Preserve this clean unloaded state as the benchmark baseline. Record the SSD’s 17 unsafe shutdowns and avoid abrupt power loss during long model loads. |

## Host identity and trust boundary

| Item | Observed |
|---|---|
| SSH alias | `nimo-2` -> management LAN `192.168.1.163:22`, user `connorb` |
| Validated hostname | `nimo-2` |
| System | Nimo Direct Inc. MME3L, motherboard “NIMO Mini PC” |
| Host-key receipt | Remote `/etc/ssh/ssh_host_ed25519_key.pub` and the local known_hosts entry for `192.168.1.163` both resolve to `SHA256:CEL+oTdkod6Mj4DZJqjSaLndofrMnYWq94lA3GK0+ls` (ED25519) |
| SSH policy | root login disabled; public key enabled; password and keyboard-interactive disabled; TCP and agent forwarding allowed |
| Privilege | `connorb` has unrestricted passwordless sudo |

The host identity, vendor/model, management address, and project role agree with the canonical cluster facts. No identity mismatch was found.

## Hardware and firmware evidence

### Platform

- BIOS: American Megatrends 3.05, release 2025-10-11; firmware resource version 305; AGESA StrixHaloPI-FP11 1.0.0.1c.
- CPU: AMD Ryzen AI MAX+ 395 with Radeon 8060S, 16 cores / 32 threads, boost enabled, 625 MHz-5.1875 GHz advertised.
- Topology: one Linux NUMA node, two 8-core dies/32 MiB L3 domains, 64 MiB L3 total. SMT siblings are separated by 16 logical CPU IDs.
- Memory: eight 16 GiB Samsung LPDDR5 devices, 8000 MT/s, channels A-H, 128 GiB installed. SMBIOS incorrectly reports the physical array maximum as 64 GiB; treat that field as a firmware bug.
- GPU: Radeon 8060S, gfx1151, 40 CUs, wave32, 2.9 GHz maximum, 256-bit LPDDR5, 1 GiB firmware VRAM plus 124 GiB GTT.
- NPU: AMD Strix Halo amdxdna device is present on PCIe, but driver initialization fails because IOMMU is disabled.
- Management NIC: Realtek RTL8125 using `r8169`, negotiated 2.5 Gb/s full duplex.
- Wi-Fi: MediaTek MT7925 Wi-Fi 7; interface present but down.
- Storage: Crucial P310 1 TB DRAM-less NVMe, PCIe 4.0 x4, firmware VACR001.

### Firmware/security-visible state

- UEFI boot through GRUB 2.14.
- Secure Boot disabled; kernel lockdown `none`.
- TPM 2 device present.
- IOMMU hardware exists, but the boot option disables it and the live system has no IOMMU groups.
- Kernel taint is 0.
- BIOS “Performance” mode and iGPU framebuffer “Auto” were recorded by the earlier validated project run, but Linux does not expose those setup-menu values directly; this audit could not independently read them.
- No firmware-update availability query was performed because the audit was intentionally offline/read-only; only installed versions were read.

## OS, kernel, and boot policy

- OS: CachyOS rolling.
- Kernel: `7.1.3-1-cachyos`; LTS fallback package `6.18.37-1`.
- Microcode: `0xb700037`; Linux firmware/AMD ucode package date 2026-06-22.
- Boot arguments:

  `nowatchdog nvme_load=YES splash loglevel=3 amdgpu.gttsize=126976 ttm.pages_limit=32505856 ttm.page_pool_size=32505856 amd_iommu=off zswap.enabled=0 processor.max_cstate=2 pcie_aspm=off`

- The kernel warns that `amdgpu.gttsize` is deprecated and recommends `ttm.pages_limit`.
- `pcie_aspm=off` disables ASPM on the internal GPU/USB4 bridge path. Some discrete endpoints still report firmware-enabled L1.
- CPU idle is limited to POLL/C1/C2; C2 latency is 18 us. All CPU PM-QoS resume-latency files contain 100 us.

These settings should be treated as a named benchmark profile, not as an assumed universal optimum. Their effect must be measured against a control profile, particularly after moving from RCCL tensor parallel to replica/pipeline execution.

## CPU, memory, NUMA, and allocation policy

| Setting | Live value |
|---|---|
| NUMA | one node; CPUs 0-31; node size about 127,433 MB |
| MemTotal / MemAvailable | 130,491,700 kB / about 128,506,176 kB at collection |
| Swap | none |
| THP | `always`; 2 MiB size inherits; defrag `defer+madvise` |
| Static hugetlb | none reserved |
| Swappiness | 1 |
| Page cluster | 0 |
| Compaction proactiveness | 0 |
| NUMA balancing | 0 |
| Dirty thresholds | 64 MiB background / 256 MiB foreground |
| max_map_count | 1,048,576 |
| GTT / TTM | 124 GiB exact; 18,677,760 bytes used while unloaded |
| AMD system-memory limit | disabled live (`no_system_mem_limit=Y`) |
| Default mempolicy | default, node 0 |

The one-node UMA layout is favorable: there is no cross-NUMA memory placement problem. The main concern is capacity accounting. The GTT aperture being 124 GiB does not make 124 GiB safely allocatable to weights and KV. The fork should expose a hard reserve and fail admission before the kernel OOM path.

## GPU, ROCm, HIP, Vulkan, and llama.cpp

### GPU runtime

- `rocminfo`: gfx1151, Radeon 8060S, 40 CUs, wavefront 32, 2 MiB L2, 32 MiB GPU L3, 64 KiB LDS/group, 126,976 MiB coarse and fine-grained pools.
- ROCm/HIP: 7.2.4 packages; HIP tool version `7.2.53211-3d9ef42`; AMD clang 22.0.0git.
- ROCm libraries present: hipBLAS, rocBLAS, RCCL, HSA runtime.
- Missing development/runtime components relevant to the plan: hipBLASLt and rocWMMA headers/libraries.
- Device permissions: `/dev/kfd` and `/dev/dri/renderD128` are mode 0666; the user is also in `render` and `video`.
- Idle live state: no KFD process, GPU 0% busy, 42-43 C, about 15-18 W whole-SoC reading, forced `high` level at 2.9 GHz.

### Vulkan

- Mesa/RADV 26.1.4, Vulkan 1.4.354 device API.
- Integrated GPU recognized as `AMD Radeon 8060S Graphics (RADV STRIX_HALO)`.
- Subgroup size 64 by default, controllable 32-64.
- Cooperative matrices, shader FP16/int8, and 8/16-bit storage are exposed.
- Reported heaps are about 41.67 GiB and 83.33 GiB; per-allocation/max-buffer limits should be validated with llama.cpp rather than inferred from the summary.

### Retained llama.cpp RPC runtime

- Commit: `8f114a9b573b69035299f9b924047f53c1e22c7e`.
- Version output: `version: 1 (8f114a9b)`, built with GCC 16.1.1.
- Build intent recovered from the preserved script:

  - `GGML_HIP=ON`
  - `GGML_RPC=ON`
  - `GGML_HIP_ROCWMMA_FATTN=ON`
  - `AMDGPU_TARGETS=gfx1151`
  - shared libraries, Release build.

- Available device: `ROCm0: AMD Radeon 8060S Graphics (126976 MiB)`.
- Backends present: CPU, HIP, RPC. Vulkan is absent.
- The copied binaries need `LD_LIBRARY_PATH=/opt/llm-usb4-cluster/llama:/opt/rocm/lib`; direct invocation without the launcher cannot locate sibling libraries.
- `ggml-rpc-server` exposes host, port, device, threads, and local file-cache options. It remains the unmodified RPC proof baseline.
- The source checkout and CMake cache were deleted after packaging, so the binary cannot be audited or patched in place.

## USB4 and networking

### Physical/logical topology

Both USB4 controllers are live and connected to `nimo-1`:

| nimo-2 interface | Local USB4 domain | Peer | Address pair | Negotiated link |
|---|---|---|---|---|
| `thunderbolt0` | domain1 / host router c7:00.6 | `nimo-1` | `10.44.0.2/30 <-> 10.44.0.1/30` | 20.0 Gb/s x2 lanes, 40 Gb/s full duplex |
| `thunderbolt1` | domain0 / host router c7:00.5 | `nimo-1` | `10.44.0.6/30 <-> 10.44.0.5/30` | 20.0 Gb/s x2 lanes, 40 Gb/s full duplex |

Both use MTU 9000, `fq_codel`, one RX/TX queue, TSO/GSO/GRO enabled, TX checksum offload, and no RX checksum offload. Interface error/drop counters were essentially clean (zero RX errors/drops and only single-digit historical TX drops).

The current unloaded state is intentionally:

- both links up and idle;
- MPTCP kernel support enabled, but endpoint list empty and limits `subflows 0 add_addr_accepted 0`;
- MiniMax RPC and dual-MPTCP services inactive/disabled;
- no listener on 50052.

That is not a fault. The preserved service will recreate the two-subflow policy on reload.

### Latency and throughput evidence

- Audit ICMP sample: 0% loss; mean 0.133 ms and 0.131 ms.
- Controlled historical native TCP busy-polled test: about 16 us RTT / 8 us one-way.
- Controlled historical TCP payload:

  - about 10.35 Gb/s per rail;
  - 20.705 Gb/s simultaneous aggregate;
  - zero retransmissions in the final test.

- Controlled RCCL results:

| BF16 all-reduce | Single rail LL128 | Dual rail LL128 | Interpretation |
|---:|---:|---:|---|
| 4 KiB | 64.41 us | 64.81 us | no gain |
| 16 KiB | 109.16 us | 118.31 us | dual is worse |
| 64 KiB | 138.95 us | 140.70 us | no gain |
| 256 KiB | 234.04 us | 244.58 us | dual is worse |
| 1 MiB | 10.94 Gb/s | 15.36 Gb/s | bulk gain |
| 4 MiB | 11.55 Gb/s | 20.15 Gb/s | large bulk gain |
| 64 MiB | 11.82 Gb/s | 17.57 Gb/s | large bulk gain |

This measured curve is the strongest evidence against per-layer tensor collectives and for persistent graphs plus large, coalesced boundary transfers.

### IRQ, queue, and socket tuning

- No irqbalance service is installed.
- USB4 controller c7:00.5 data interrupts are concentrated on effective CPUs 31 and 0; c7:00.6 data interrupts on CPUs 29 and 30. Control interrupts are on CPUs 20-24.
- RPS masks are zero, RPS flow count is zero, global RPS socket flow entries are zero, and XPS masks are unset.
- `net.core.rmem_max=4 MiB`, `wmem_max=4 MiB`, `netdev_max_backlog=4096`.
- TCP uses cubic; receive autotuning max 32 MiB; send max 4 MiB.
- `net.core.busy_read=100`, `net.core.busy_poll=100`, TCP low latency and fast-open enabled.
- `thunderbolt_net.e2e=Y`; core DMA credits remain the module default 14.

Recommended experiment sequence:

1. collect IRQ, softirq, scheduler, copy, and retransmit counters during a fixed per-rail transfer;
2. pin control and bulk IRQs deliberately, keeping them off model orchestration threads;
3. A/B no RPS versus a small physical-core RPS mask;
4. sweep one/few large application buffers and persistent sockets;
5. retain only changes that improve payload without regressing 4-256 KiB latency.

## Storage and filesystems

- Single 1 TB Crucial P310 NVMe, PCIe 4.0 x4.
- Root is Btrfs with zstd:1, noatime, async discard, separate subvolumes; about 411 GiB used / 521 GiB available.
- Btrfs metadata is DUP; all device error counters are zero.
- Scheduler is `kyber`, 256 requests, 256 KiB read-ahead/max sector, writeback cache, WBT 2 ms.
- Weekly fstrim is enabled.
- SMART: passed; 0% endurance used; 35.5 TB read, 5.08 TB written; 0 media/integrity errors; 0 error-log entries; 17 unsafe shutdowns; 30 C.
- Filesystem is not encrypted.

NVMe is not the decode bottleneck after weights are resident. It matters for load/reload, cache persistence, crash safety, and reproducible model staging.

## Services, load, and current tuning

### Live unloaded baseline

- Load average: 0.07 / 0.04 / 0.07.
- CPU, memory, and I/O PSI were zero at the instant of collection apart from small historical totals.
- No failed systemd units and no coredumps.
- No KFD users, llama process, Ray worker, vLLM service, or RPC listener.
- Active long-running services are basic OS/network services only.

### Active tuning one-shots

| Unit | State | Effect |
|---|---|---|
| `amdgpu-llm-tune.service` | enabled, active/exited | GPU power control on and performance level high |
| `llm-cpu-performance.service` | enabled, active/exited | CPU governor and EPP performance |
| `llm-memory-sysctl.service` | enabled, active/exited | low-swappiness/map-count memory profile |
| `llm-usb4-pmqos.service` | enabled, active/exited | CPU resume-latency QoS 100 us |
| `step37-performance-tuning.service` | disabled but active/exited | duplicates CPU/GPU/PM-QoS and forces USB4 power on |

### Preserved workload units

- MiniMax RPC/MPTCP units: intentionally disabled and inactive per unload receipt.
- Step-3.7 Ray/container units: disabled and inactive.
- DwarfStar toolbox container: present, exited.
- Several service definitions no longer match present paths/containers; they should be labeled historical or repaired before being offered as rollback targets.

## Security and operational constraints

- SSH is key-only and root login is disabled, which is good.
- The login user has unrestricted NOPASSWD root, so compromise of that account is full-host compromise.
- Secure Boot, disk encryption, kernel lockdown, and live IOMMU isolation are absent.
- `/dev/kfd` and the render node are world read/write despite group membership already being configured.
- Firewalld’s management zone permits only SSH, but the private USB4 zone accepts all inbound services from the peer.
- UFW and firewalld both program netfilter. This makes effective policy harder to reason about and should be simplified before exposing a new backend.
- Global mDNS/LLMNR services are active; not performance-significant, but unnecessary discovery should be removed from private transport interfaces if the cluster threat model requires strict isolation.

Before any new API, RPC worker, router, or transport listener starts, firewall
and bind-address validation is a hard gate: install explicit rules first, prove
the service is restricted to the intended USB4/private address, confirm
management SSH in a second session or at console, verify management-LAN
rejection and peer-USB4 acceptance, and retain tested rollback commands.

### Cross-node symmetry and asymmetry

| Area | Shared state | Relevant difference |
|---|---|---|
| Platform | MME3L, BIOS 3.05, kernel 7.1.3, ROCm 7.2.4 generation, 124 GiB GTT cap, two 40 Gb/s rails, performance/high policy, IOMMU off, no direct-stream interface | None material to hardware identity |
| Memory safety | Both use the large GTT/TTM profile | nimo-1 retains a 32 GiB swapfile with zswap enabled; nimo-2 has no swap and explicitly disables zswap |
| Build dependencies | Both run the copied HIP runtime | nimo-1 has hipBLASLt and rocWMMA packages; nimo-2 lacks these new-build dependencies, although `rocm-hip-libraries` is installed |
| Firewall | Both currently have no model listener | nimo-1 uses UFW; nimo-2 has UFW and firewalld simultaneously, with firewalld `llm-usb4` target `ACCEPT` |
| Crash history | Neither had an active crash during this audit | nimo-1 retains three historical vLLM EngineCore coredumps; nimo-2 has none, reflecting process placement/history rather than intrinsic worker stability |
| USB4 bulk IRQs | Both concentrate bulk processing on a few logical CPUs | Placement is mirrored: nimo-1 c7:00.5 uses CPUs 29/30 and c7:00.6 uses 31/0; nimo-2 reverses those controller assignments |

rocWMMA is a build/header dependency. hipBLASLt is a build/runtime library
dependency for the proposed comparison path. Their absence on nimo-2 does not
prevent the copied current HIP binary from running, but it blocks a like-for-like
new build until dependencies are aligned or the feature is disabled explicitly.

## Health/log findings

Current-boot warnings with direct relevance:

1. `amdxdna` NPU probe fails because IOMMU is disabled.
2. USB4NET receives the kernel “suspect GRO implementation” warning.
3. `amdgpu.gttsize` is deprecated.
4. Several early USB4 retimer/host disconnect/reconnect events occurred during the known physical topology work; both rails are now present and clean.
5. Podman overlay-on-Btrfs capability warnings are present but did not cause an active workload fault.

No GPU reset, OOM, MCE, NVMe error, Btrfs corruption, kernel panic, coredump, or failed unit was observed.

## Safe/reversible next changes

These are candidates, not actions taken by this audit:

1. Create reproducible HIP and Vulkan builds from one pinned source commit; embed commit, CMake options, compiler/library versions, RPATH, and patch ID.
2. Install a pinned profiler/benchmark bundle: Linux perf, ROCm profiler/compute profiler, rocgdb, fio, sysstat, a real STREAM/MLC-like memory benchmark, trace-cmd/bpftrace as appropriate, and nvme-cli.
3. Add runtime lifecycle scripts that apply and restore CPU/GPU/PM-QoS/busy-poll tuning.
4. Restore `amdgpu.no_system_mem_limit=N` when no large mapping is required, after confirming the unloaded state remains healthy.
5. Archive or clearly label stale service units; add path/image validation to every reload unit.
6. Add application-level telemetry for graph submission latency, queue depth, buffer reuse, copy counts, bytes/token, per-rail traffic, back-pressure, and pipeline idle time.
7. A/B IRQ affinity/RPS per rail with a fixed latency-and-throughput acceptance gate.
8. Use a small local model corpus to validate local replica routing, session/KV affinity, continuous batching, and quantized KV quality before distributed work.

## Risky or reboot-required changes

1. Enable IOMMU and remove `amd_iommu=off`; validate HSA large mappings, GPU throughput, USB4 stability, NPU probe, and security state.
2. Remove or relax `processor.max_cstate=2` and `pcie_aspm=off`; compare busy-polled latency, decode throughput, idle power, and link stability.
3. Remove deprecated `amdgpu.gttsize` while retaining a measured `ttm.pages_limit`; validate reported GTT and maximum model load.
4. Change the 124 GiB TTM/GTT reservation or add worker swap. Either can alter load behavior and OOM failure modes.
5. Install a patched/custom kernel for direct USB4 streaming.
6. Change BIOS performance/framebuffer settings or update BIOS, retimer, NVMe, or other firmware.
7. Apply GPU overdrive, power-cap, deterministic clock, memory-clock, or voltage changes.
8. Converge UFW/firewalld or change default firewall policy. Apply explicit
   management and USB4 rules first, keep a second SSH session or console
   available, validate intended accept/reject paths, and retain rollback commands.

Every item above needs a before/after profile and rollback entry. Do not combine boot-variable experiments in one reboot.

## Required benchmark matrix

### Baseline

- Record exact OS/kernel/BIOS/ROCm/Mesa/llama commit and patch set.
- Record power profile, boot arguments, GPU performance level, `no_system_mem_limit`, THP, swap, IRQ affinity, RPS/XPS, busy-poll, and firewall state.
- Capture idle power/temperature/load, then a fixed warm-up.

### Local backend

- HIP and Vulkan separately.
- Dense and MoE models.
- Q4_K, Q5_K, Q6_K, Q8_0 and any target FP8 path.
- Decode batch 1 and continuous-batch/concurrency sweep.
- Prefill shapes across several contexts and microbatches.
- FlashAttention on/off/auto by shape.
- Metrics: TTFT, prompt tok/s, decode tok/s, aggregate tok/s, p50/p99, GPU busy, clocks/power, kernel occupancy, VGPR/LDS, memory throughput, launch count, and CPU orchestration cost.
- Specifically profile `mul_mat_vec_q`, `quantize_q8_1`, dequant/GEMV, RMSNorm, RoPE, gating/SiLU, reductions, expert selection, and grouped GEMM.

### Transport

- Per-rail and simultaneous: 1 byte through at least 64 MiB.
- One stream and controlled parallel streams, both directions.
- Persistent connection versus reconnect, TCP versus MPTCP, copied versus pinned/aligned buffers.
- CPU/IRQ/softirq/copy cost, retransmits, queue depth, back-pressure, and bytes submitted/completed.
- Current USB4NET versus any direct-stream prototype under the same payload/latency suite.

### Distributed modes

- Replica: single and concurrent requests, strict session/KV affinity, aggregate target 1.7-1.9x.
- Pipeline: one contiguous boundary; sweep boundary placement and microbatch interleave; report activation bytes/token and stage idle time.
- Speculative: draft size/model, acceptance rate, verification batch, inter-node bytes/output token, and end-to-end single-stream gain.
- Tensor split only as a negative/control benchmark using the already-known RCCL curve.
- Never migrate KV during normal decode. If KV quantization is enabled, run per-model quality/perplexity and long-context regression checks.

## Unknowns and evidence gaps

- No active model was loaded, so this audit did not measure decode/prefill kernels, sustained loaded power/thermals, or current llama.cpp throughput.
- Linux cannot confirm the current BIOS menu selections; only firmware version and prior project receipts are available.
- No physical memory bandwidth benchmark is installed, so 256 GB/s is theoretical.
- No current GPU profiler is installed, so hot-kernel, occupancy, cache, VGPR, and launch conclusions remain profiling leads.
- The running kernel does not expose a direct streaming API; feasibility, upstream status, and patch maintenance need a separate source/kernel investigation.
- USB4 retimer firmware versions were not exposed through the inspected sysfs/bolt interfaces.
- Vulkan heap/allocation behavior for a 100+ GiB llama.cpp workload has not been validated.
- Quantized KV quality, speculative acceptance, and MoE placement require model-specific tests.
- Firmware update availability was not queried.

## Command receipt

The transient Base64 payload strings and their byte-for-byte decoded scripts
were not archived during collection, so that original transport representation
is explicitly unavailable. The earlier placeholder summary was not an exact
receipt and has been removed. The commands below are the complete,
placeholder-free, read-only rerun set for the evidence used in this report.
Repeated per-object calls are expressed as executable discovery loops rather
than guessed object identifiers. Redaction applies to captured output, not to
the commands.

The SSH target and host identity were checked with these exact commands. The
remote and locally recorded ED25519 fingerprints both resolved to
SHA256:CEL+oTdkod6Mj4DZJqjSaLndofrMnYWq94lA3GK0+ls.

```powershell
Resolve-DnsName nimo-2 -Type A
ssh -o BatchMode=yes -o ConnectTimeout=10 nimo-2 "hostname; sudo -n ssh-keygen -lf /etc/ssh/ssh_host_ed25519_key.pub"
$knownLine = ssh-keygen -F 192.168.1.163 -f "$env:USERPROFILE\.ssh\known_hosts" | Select-Object -Last 1
$knownKey = $knownLine.Split(' ', [System.StringSplitOptions]::RemoveEmptyEntries)[2]
$knownKey | ssh-keygen -lf -
```

Because the login shell is fish, each Bash block was assigned literally to
the PowerShell variable $script and transported without creating a remote file:

```powershell
$payload = [Convert]::ToBase64String([Text.Encoding]::UTF8.GetBytes($script))
ssh -o BatchMode=yes -o ConnectTimeout=10 nimo-2 "printf '%s' '$payload' | base64 -d | bash"
```

Identity, firmware, CPU, memory, and kernel:

```bash
set -o pipefail
hostnamectl
id
sudo -n true
cat /proc/cmdline
uname -a
cat /etc/os-release
bootctl status --no-pager
lsmod
sudo -n dmidecode -t bios -t system -t baseboard
sudo -n dmidecode -t memory
lscpu
lscpu --caches
lstopo-no-graphics --of console
numactl --hardware
numactl --show
free -h
grep -E '^(MemTotal|MemFree|MemAvailable|Buffers|Cached|SwapCached|Active|Inactive|SwapTotal|SwapFree|Dirty|Writeback|AnonPages|Mapped|Shmem|KReclaimable|Slab|SReclaimable|SUnreclaim|KernelStack|PageTables|Committed_AS|VmallocTotal|VmallocUsed|Percpu|HardwareCorrupted|HugePages_Total|HugePages_Free|HugePages_Rsvd|HugePages_Surp|Hugepagesize|Hugetlb):' /proc/meminfo
for f in /sys/kernel/mm/transparent_hugepage/enabled /sys/kernel/mm/transparent_hugepage/defrag /sys/kernel/mm/transparent_hugepage/shmem_enabled; do
    printf '\n[%s]\n' "$f"
    cat "$f"
done
for key in vm.swappiness vm.max_map_count vm.dirty_background_ratio vm.dirty_ratio net.core.rmem_max net.core.wmem_max net.core.netdev_max_backlog net.core.netdev_budget net.core.netdev_budget_usecs net.core.busy_read net.core.busy_poll net.core.rps_sock_flow_entries net.ipv4.tcp_congestion_control net.ipv4.tcp_rmem net.ipv4.tcp_wmem net.ipv4.tcp_fastopen net.ipv4.tcp_low_latency; do
    sysctl "$key"
done
```

PCIe, IOMMU, USB4, and Thunderbolt:

```bash
set -o pipefail
lspci -nnk
lspci -tv
while read -r dev; do
    printf '\n[PCI %s]\n' "$dev"
    sudo -n lspci -s "$dev" -vv
done < <(lspci -D | awk '/AMD.*(VGA|Display|Audio|Encryption|USB|Host bridge|PCI bridge|Non-Volatile|Signal processing)|Thunderbolt|USB4/ {print $1}')
find /sys/kernel/iommu_groups -maxdepth 3 -type l -print -exec readlink -f '{}' ';'
ls -la /sys/bus/thunderbolt/devices
for d in /sys/bus/thunderbolt/devices/*; do
    test -d "$d" || continue
    printf '\n[THUNDERBOLT %s]\n' "$(basename "$d")"
    for a in authorized device device_name generation key nvm_version rx_lanes security unique_id vendor vendor_name; do
        test -r "$d/$a" || continue
        printf '%s=' "$a"
        cat "$d/$a"
    done
done
boltctl list --all
boltctl domains
modinfo thunderbolt
modinfo thunderbolt_net
test -r /sys/module/thunderbolt_net/parameters/e2e && cat /sys/module/thunderbolt_net/parameters/e2e
```

Network, queues, IRQs, and rail latency:

```bash
set -o pipefail
ip -br link
ip -br addr
ip route show table all
ip rule show
ip mptcp endpoint show
ip mptcp limits show
nmcli -t -f NAME,UUID,TYPE,DEVICE connection show --active
ss -s
sudo -n ss -lntupH
for i in $(find /sys/class/net -mindepth 1 -maxdepth 1 -printf '%f\n' | sort); do
    printf '\n[INTERFACE %s]\n' "$i"
    ethtool "$i" 2>&1 || true
    ethtool -k "$i" 2>&1 || true
    ethtool -l "$i" 2>&1 || true
    ethtool -g "$i" 2>&1 || true
    ethtool -c "$i" 2>&1 || true
    tc -s qdisc show dev "$i"
    for q in /sys/class/net/$i/queues/rx-* /sys/class/net/$i/queues/tx-*; do
        test -d "$q" || continue
        for f in rps_cpus rps_flow_cnt xps_cpus xps_rxqs; do
            test -r "$q/$f" || continue
            printf '%s/%s=' "$(basename "$q")" "$f"
            cat "$q/$f"
        done
    done
done
sudo -n grep -Ei 'thunderbolt|amdgpu|xhci|nvme|kfd' /proc/interrupts
awk -F: '/thunderbolt|amdgpu|xhci|nvme|kfd/ {gsub(/[[:space:]]/, "", $1); print $1}' /proc/interrupts |
while read -r irq; do
    case "$irq" in
        ''|*[!0-9]*) continue ;;
    esac
    for f in smp_affinity_list effective_affinity_list; do
        test -r /proc/irq/$irq/$f || continue
        printf 'irq=%s %s=' "$irq" "$f"
        cat /proc/irq/$irq/$f
    done
done
ping -n -q -c 20 -i 0.05 -I thunderbolt0 10.44.0.1
ping -n -q -c 20 -i 0.05 -I thunderbolt1 10.44.0.5
```

Storage, GPU stacks, and retained llama.cpp runtime:

```bash
set -o pipefail
lsblk -e 7 -o NAME,KNAME,PATH,MAJ:MIN,SIZE,TYPE,FSTYPE,FSVER,MOUNTPOINTS,ROTA,SCHED,RA,MIN-IO,OPT-IO,PHY-SEC,LOG-SEC,MODEL,SERIAL
findmnt
df -hT
sudo -n btrfs filesystem usage -T /
sudo -n btrfs device stats /
sudo -n btrfs scrub status /
sudo -n smartctl -a /dev/nvme0
rocminfo
rocm-smi --showallinfo
hipconfig --version
hipcc --version
vulkaninfo --summary
vulkaninfo
clinfo
pacman -Q
ldconfig -p
find /opt/llm-usb4-cluster -maxdepth 4 -printf '%y %p %s bytes\n' | sort
LD_LIBRARY_PATH=/opt/llm-usb4-cluster/llama:/opt/rocm/lib /opt/llm-usb4-cluster/llama/llama-cli --version
LD_LIBRARY_PATH=/opt/llm-usb4-cluster/llama:/opt/rocm/lib /opt/llm-usb4-cluster/llama/llama-cli --list-devices
ldd /opt/llm-usb4-cluster/llama/llama-cli
tar -tzf /opt/llm-usb4-cluster/llama-build.tar.gz
nm -D -C /opt/llm-usb4-cluster/llama/libggml-hip.so.0.16.0
```

Services, tuning, security, and logs:

```bash
set -o pipefail
systemctl --failed
systemctl list-units --type=service --state=running
systemctl list-unit-files
for unit in amdgpu-llm-tune.service llm-cpu-performance.service llm-memory-sysctl.service llm-usb4-pmqos.service step37-performance-tuning.service llama-step37.service step37-container.service minimax-m27-q6-server.service minimax-m27-rpc-worker.service llm-dual-usb4-mptcp.service; do
    printf '\n[UNIT %s]\n' "$unit"
    systemctl show "$unit" --no-pager
    systemctl cat "$unit" --no-pager
done
systemd-analyze blame
ps -eo pid,ppid,psr,ni,stat,comm,%cpu,%mem,rss
vmstat -w 1 5
for f in /proc/pressure/cpu /proc/pressure/memory /proc/pressure/io; do
    printf '\n[%s]\n' "$f"
    cat "$f"
done
cpupower frequency-info
cpupower idle-info
sensors
sudo -n cat /sys/kernel/debug/dri/0/amdgpu_pm_info
sudo -n sshd -T
sudo -n -l
sudo -n firewall-cmd --state
sudo -n firewall-cmd --get-default-zone
sudo -n firewall-cmd --get-active-zones
for zone in $(sudo -n firewall-cmd --get-zones); do
    printf '\n[FIREWALL ZONE %s]\n' "$zone"
    sudo -n firewall-cmd --zone="$zone" --list-all
done
sudo -n firewall-cmd --direct --get-all-rules
sudo -n nft list ruleset
sudo -n ufw status verbose
sudo -n journalctl -k -b --no-pager -o short-precise
sudo -n journalctl -k -b --no-pager -o short-precise | grep -Ei 'amdgpu|kfd|thunderbolt|usb4|iommu|amdxdna|nvme|btrfs|error|fail|warn'
sudo -n journalctl -b -p err..alert --no-pager -o short-precise
coredumpctl list --no-pager
fwupdmgr get-devices --no-unreported-check
```

Local project evidence also consulted:

- `_config/cluster-facts.md`
- `_runs/2026-07-11T120832-0700__dual-usb4-port-matrix/RESULTS.md`
- `_runs/2026-07-11T205044-0700__minimax-m2.7-q6-k-xl-deployment/receipts/2026-07-12__minimax-m2.7__model-unload__receipt.md`

## Final audit state

The host remained connected and unchanged. At close of collection, both USB4 rails were up at 40 Gb/s negotiated rate, the model/RPC stack was intentionally unloaded, GPU and memory were free, and no failed unit or active workload was present.
