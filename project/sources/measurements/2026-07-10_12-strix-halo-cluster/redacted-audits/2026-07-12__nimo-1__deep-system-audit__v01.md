---
type: deep-system-audit
status: complete
date: 2026-07-12
host: nimo-1
scope: read-only
sensitivity: internal
---

# nimo-1 deep system audit

## Audit contract

- Collection window: 2026-07-12 00:38-00:53 PDT.
- Method: live SSH inspection with noninteractive read-only commands. No remote files, packages, services, settings, repositories, network policy, models, or firmware were changed.
- Local changes: this report and the minimum project manifest/changelog references required by the workspace contract.
- Redaction: raw machine/boot IDs, serial numbers, MAC addresses, credentials, private keys, API-key content, and credential-like environment values are omitted. The public SSH host-key fingerprint and private cluster addresses are retained as operational project facts.
- Authority: live state and the 2026-07-12 MiniMax unload receipt are authoritative. Older loaded/persistent README language is historical.
- Review state: independently peer-reviewed and approved after USB4STREAM, priority, reproducibility, execution-mode, causal-qualifier, and risk-class corrections; no repeat live audit was required.

## Executive assessment

nimo-1 is a healthy Strix Halo node with the expected 16-core/32-thread Ryzen AI MAX+ 395, 128 GiB across eight 16 GiB channels at 8000 MT/s, one 40-CU gfx1151 Radeon 8060S, and two independently enumerated USB4 host routers. The platform/canonical specification calls the memory LPDDR5X-8000; live dmesg labels the 256-bit interface LPDDR5 and DMI does not independently prove the X suffix. Both links to nimo-2 are trained at two 20 Gb/s lanes in each direction, report 40,000 Mb/s at the network devices, use MTU 9000, and show no interface errors. The GPU exposes exactly 126,976 MiB of GTT and the system had about 122 GiB available during the unloaded audit.

The live state is intentionally unloaded. No llama.cpp, vLLM, Ray, ROCm compute, API, or RPC process was running; the only externally reachable TCP listener on non-loopback addresses was SSH. Loopback DNS and UDP discovery remained active. MiniMax, its dual-USB4 MPTCP unit, Step-3.7, and the older llama Step-3.7 unit were disabled and inactive. The preserved MiniMax Q6 artifact is 207,445,443,072 bytes (about 194 GiB), so it does not fit on one node and cannot be the replica-mode validation model. This live state and the 2026-07-12 unload receipt supersede the older README wording that calls MiniMax persistent/loaded.

Canonical controlled tests already measured about 10.35 Gb/s TCP payload per rail, 20.705 Gb/s simultaneously, 19.2-21 Gb/s for dual-subflow MPTCP, and about 8 microseconds TCP half-RTT. The recorded warm Step-3.7 decode moved only 21.07 MB/s (0.169 Gb/s) per direction, about 0.81% of measured dual-rail capacity, and was compute-bound. These results make USB4 queue/GRO/IRQ work a measured transport-optimization target, not a proven current application bottleneck. Large collective messages benefited from the second rail; decode-sized small collectives did not.

The deepest opportunities are below the existing surface tuning:

1. Establish reproducible HIP and Vulkan builds plus transport/kernel telemetry. The retained recipe records core flags, but source, generated cache, full toolchain manifest, and a proven rebuild are absent; the required profiling tools are also missing.
2. Preserve replica/data parallel for a separate fitting model, one-boundary layer mode for the 207 GB MiniMax artifact, and speculative draft/verify as the three product modes. Keep tensor/row split deferred.
3. USB4NET is single-queue with no RSS/RPS/XPS/channel/ring/coalesce controls, and GRO is enabled despite a kernel warning. Profile it against the already strong measured baseline; do not assume it limits current decode.
4. The firewall is active but has a default allow-incoming policy. The preserved MiniMax launcher binds 0.0.0.0:8081, while live rules cover the older port 8000 and have no explicit 8081 contract. While stopped there is no API exposure; policy convergence is a pre-start gate.
5. Persistent benchmark-profile state remains while unloaded: amdgpu no_system_mem_limit=Y, GPU high clocks, CPU performance/EPP with a 2 GHz floor, and active/exited tuning one-shots. The unload receipt did not promise a balanced state, so this is lifecycle/reproducibility drift rather than a failed unload.
6. The installed 7.1.3 source/config has no USB4_CONFIGFS/USB4_STREAM support, thunderbolt_stream module, ConfigFS USB4 stream tree, or /dev/tbstreamX device. This does not block replica or USB4NET layer mode; it makes upstream USB4STREAM an optional later kernel experiment.
7. amd_iommu=off removes IOMMU groups, causes amdxdna/NPU initialization to fail, and reduces DMA isolation. llama.cpp's separate VMM: no observation is not attributed to IOMMU without an A/B.

No remote configuration, repository, service, network, boot, or model change was made during this audit. Diagnostic commands initialized ROCm/Vulkan discovery and naturally changed access logs and live counters, but no benchmark or sustained workload was run.

## Identity and trust validation

- Local SSH alias: connorb@nimo-1.local, port 22, key path ~/.ssh/id_ed25519.
- Remote hostname: nimo-1. The address set was 192.168.1.55/24, 10.44.0.1/30, and 10.44.0.5/30.
- The remote ED25519 public-host-key fingerprint and the local known_hosts entry matched exactly: SHA256:rOQQA0dAirWpqwKyGVkvG8V9k4q8sD7CUPqTEmoSnRA.
- Login was non-interactive public-key SSH as connorb. Password authentication and root login are disabled. connorb has non-interactive sudo.
- Stable machine and boot identifiers were hashed for validation during collection and are intentionally omitted here.

## Ranked findings

| Rank | Severity | Finding and evidence | Consequence | Recommended disposition |
|---:|---|---|---|---|
| 1 | High security | UFW is active but Default: allow (incoming). The current rules explicitly protect port 8000, not MiniMax port 8081; the preserved launcher binds 0.0.0.0:8081. | Starting the API can expose it to every reachable source not rejected elsewhere. Future fork daemons could be exposed similarly. | Treat firewall convergence as a pre-start gate. During a controlled console/two-session window, define explicit SSH, management-API, and private-rail rules before switching to default deny; validate recovery and effective nftables policy. |
| 2 | High engineering blocker | perf, ROCm profiler tools, Omniperf/rocprof-compute, amd-smi, rocgdb, BPF tracing, CPU/IO telemetry, and latency microbench tools are absent. rocm-smi also throws a map::at exception for some metrics. | Kernel, launch, occupancy, bandwidth, queueing, and copy-count claims cannot be measured reproducibly. | Approve and install a pinned diagnostic/tooling manifest before optimization; record versions, permissions, and a smoke test. |
| 3 | High reproducibility | The retained runtime is commit 8f114a9b and the canonical script records GGML_HIP=ON, GGML_RPC=ON, GGML_HIP_ROCWMMA_FATTN=ON, AMDGPU_TARGETS=gfx1151, shared Release/Ninja. Source, generated CMake cache, full toolchain manifest, and a proven rebuild are absent. | Patch/rebase claims and backend comparisons are not reproducible from the binary package alone. | Build the fork in a separate source tree with lockfile/commit/build manifest, symbols, HIP and Vulkan trees, then prove byte/version-identifiable rebuilds. |
| 4 | Medium transport optimization | Both USB4 devices have one RX and one TX queue, RSS fixed off, rps_cpus=0, rps_flow_cnt=0, xps_cpus=0, and no channel/ring/coalesce/stat API. GRO/GSO/TSO are on, and the kernel emitted the suspect-GRO warning. Canonical tests nevertheless reached 10.35 Gb/s/rail, 20.705 Gb/s aggregate, and ~8 us half-RTT; warm Step-3.7 decode used only 0.81% of capacity. | Queue/GRO/IRQ work may improve bulk transport or tail latency, but it is not proven to limit current inference. | A/B GRO, IRQ/application affinity, targeted RPS, qdisc, and batching against the measured latency/throughput baseline. Reject changes that improve bulk at the cost of decode-sized p99. |
| 5 | Medium lifecycle/reproducibility | While intentionally unloaded, no_system_mem_limit=Y, GPU performance level=high, all 32 CPU policies use performance/EPP performance with a 2 GHz minimum, and latency one-shots remain active. Idle SoC power was about 17 W. The Changelog says the primary override was restored to N, so current live Y is a documentation/state contradiction. | New benchmarks inherit an implicit historical profile and idle power stays elevated. Y is not itself proven harmful. | Define explicit idle and inference profiles; capture prior state and test reversible apply/restore hooks before changing the known-good reload path. Correct the stale Changelog assertion. |
| 6 | Medium compatibility/security | Boot has amd_iommu=off; there are zero IOMMU groups and amdxdna fails with Running without IOMMU not supported. llama-bench separately reports VMM: no; causality was not established. | NPU is unusable and DMA isolation is reduced. GPU VMM implications remain unknown. | Reboot A/B IOMMU on versus off with ROCm allocation, llama HIP/Vulkan, USB4, and latency tests. Retain off only with measured necessity. |
| 7 | Medium profiling target | USB4 data IRQs currently land on CPUs 29/30 for one controller and 31/0 for the other. irqbalance is absent/masked; llama threads have no matching CPUAffinity. | Network softirq and inference/HTTP threads may collide, especially at concurrency, but current decode was not link-saturated. | Profile first; then A/B reserved rail cores and process affinity. Keep only improvements in p50/p99 and aggregate throughput. |
| 8 | Medium deferred kernel feature | The running source/config has USB4 and USB4NET only; it has no USB4_CONFIGFS/USB4_STREAM option, thunderbolt_stream module, ConfigFS stream tree, or /dev/tbstreamX. CONFIG_USB4_DMA_TEST is merely a loopback test and is not a stream prerequisite. | Upstream USB4STREAM cannot be exercised on this kernel, but its absence does not block replica or USB4NET layer mode. | First test a supported kernel with CONFIG_USB4_STREAM. Backport only if justified, preserving USB4NET. Model its actual read/write/poll character-device ABI, mandatory E2E flow control, <=4 KiB DATA frames, and copy behavior. |
| 9 | Medium stability | Three VLLM::EngineCore processes crashed on 2026-07-11 (two SIGSEGV, one SIGABRT), with coredump storage disabled. | Prior FP8 instability lacks actionable stack evidence. | Capture bounded coredumps or ROCm fault telemetry for the next controlled reproduction, while protecting model/API secrets. |
| 10 | Medium performance | THP is always, no hugetlb pages are reserved, zswap is enabled with zstd and a 20% pool, a 32 GiB Btrfs swapfile remains, and the NVMe uses kyber plus 2 ms writeback throttling. | These may help the oversized non-mmap loader, but can also spend CPU/RAM, compact memory, or slow sequential loading. | A/B THP, zswap, swap behavior, and NVMe scheduler only with model-load time, memory pressure, TTFT, and stability evidence. |
| 11 | Low-medium boot hygiene | amdgpu.gttsize=126976 is still present even though the driver says it is deprecated and ttm.pages_limit should be used. processor.max_cstate=2 and pcie_aspm=off are global. | Redundant/deprecated arguments complicate upgrades; global latency knobs increase idle power and may not improve the hot path. | In a reboot matrix, remove only the deprecated duplicate first; separately test C-state and ASPM effects on rail RTT, TTFT, and watts. |
| 12 | Low firmware data quality | DMI exposes eight 16 GiB devices at 8000 MT/s but the Type 16 structure says maximum capacity 64 GiB. Linux correctly sees 128 GiB. fwupd is masked and BIOS profile is not exposed through platform_profile. | Automated inventory can misclassify capacity; live Linux cannot prove the selected BIOS Performance mode. | Trust OS/SMU measurements for capacity; verify BIOS mode manually or under sustained power testing. Check vendor firmware notes before any update. |

### Evidence map for ranked findings

| Finding | Principal command/evidence | Sanitized observed value |
|---:|---|---|
| 1 | sudo ufw status verbose; ss -lntup; systemctl show minimax-m27-q6-server.service; launcher inspection | UFW default incoming allow; no current API listener; preserved launcher binds 0.0.0.0:8081; live rules cover 8000, not 8081 |
| 2 | command -v tool matrix; selected --version probes | perf/rocprof/Omniperf/amd-smi/rocm debugger/BPF tracing/fio/sysstat/latency tools absent; iperf3/ROCm discovery/hwloc present |
| 3 | logs/llama-commit.txt; logs/llama-build.log; canonical scripts/50-build-llamacpp-rocm.sh; find for .git/CMakeCache | commit and core CMake recipe retained; source, generated cache, and reproducible rebuild absent |
| 4 | ethtool -k/-l/-g/-c/-S; queue sysfs; journalctl -k; canonical dual-USB4 and warm Step-3.7 results | one queue/rail, no RSS/RPS/XPS controls, GRO on, suspect-GRO warning; 20.705 Gb/s aggregate/~8 us half-RTT; decode 0.81% link use |
| 5 | amdgpu sysfs/module parameters; cpufreq policies; systemctl show tuning units; sensors/amdgpu_pm_info | no_system_mem_limit=Y; GPU high; CPU performance/EPP and 2 GHz floor; ~17 W idle; model/API/RPC unloaded |
| 6 | /proc/cmdline; IOMMU group count; boot journal; llama-bench discovery | amd_iommu=off; zero groups; amdxdna probe failure; independent llama VMM: no observation |
| 7 | PCI MSI IRQ sysfs, /proc/interrupts, /proc/irq affinity files, queue sysfs | bulk vectors effective on CPUs 29/30 and 31/0; RPS/XPS zero; irqbalance absent |
| 8 | /proc/config.gz; installed thunderbolt Kconfig/modules; ConfigFS and /dev inspection | current source contains USB4 and DMA loopback test only; no USB4_CONFIGFS/USB4_STREAM symbol, thunderbolt_stream, stream ConfigFS tree, or tbstream device |

## Hardware and topology evidence

### Platform

| Component | Live evidence |
|---|---|
| System | Nimo Direct MME3L / NIMO Mini PC |
| BIOS | AMI 3.05, released 2025-10-11 |
| Boot | UEFI/GRUB, Secure Boot disabled, TPM2 present, kernel lockdown none |
| OS/kernel | CachyOS; Linux 7.1.3-1-cachyos, PREEMPT_DYNAMIC, Clang/LLD 22 kernel build |
| CPU | Ryzen AI MAX+ 395, 16 cores / 32 threads, two 32 MiB L3 dies, boost enabled, 5.1875 GHz advertised maximum |
| NUMA | One NUMA node; all CPUs and 124 GiB usable memory in node 0 |
| Memory | Eight 16 GiB Samsung K3KLALA0EM-MGCV devices, 8000 MT/s; dmesg identifies a 256-bit LPDDR5 interface. Platform/canonical spec calls it LPDDR5X-8000; live inventory does not prove the X suffix |
| GPU | Radeon 8060S, PCI ID 1002:1586 rev c1, gfx1151, 40 CUs, wave32, 2.9 GHz maximum, one SDMA engine |
| GPU memory | 1 GiB firmware VRAM plus 126,976 MiB GTT; 256-bit UMA. Audit use: ~155 MiB VRAM and 18 MiB GTT |
| GPU PCI path | Internal PCIe 16 GT/s x16, amdgpu driver, 1 GiB BAR |
| NPU | Present at c6:00.1 but amdxdna probe fails because IOMMU is off |
| NVMe | Crucial P310 1 TB DRAM-less, firmware VACR001, PCIe/NVMe 2.0 |

### Idle health

- Load average was effectively zero and no inference process was resident.
- GPU edge was 43-44 C; CPU Tctl 45.5 C; NVMe 32 C; no thermal warning time was recorded.
- SoC package reading was approximately 17 W at idle with the high-clock policy still applied.
- NVMe SMART: critical warning 0, spare 100%, percentage used 1%, zero media/integrity errors, zero error-log entries, and 14 lifetime unsafe shutdowns.
- Btrfs device statistics show zero read, write, flush, corruption, or generation errors.

## Kernel, boot, and firmware-visible policy

Current kernel command line:

    nowatchdog nvme_load=YES splash loglevel=3
    amdgpu.gttsize=126976
    ttm.pages_limit=32505856
    ttm.page_pool_size=32505856
    amd_iommu=off
    processor.max_cstate=2
    pcie_aspm=off

The same line is persisted in /etc/default/grub and both normal/LTS GRUB entries. Relevant observations:

- amdgpu successfully creates 126,976 MiB GTT, but explicitly warns that gttsize is deprecated.
- IOMMU is effectively off: no groups exist and the NPU driver fails.
- CPU idle exposes POLL, C1, and C2 only. CPU0 C2 latency is 18 microseconds.
- PCIe ASPM is disabled at both USB4 host routers and globally.
- Secure Boot and kernel lockdown are off. TPM2 exists but the OS is not measured as a UKI.
- fwupdmgr is installed but cannot contact its masked daemon; update availability was not checked.
- Kernel warnings relevant to this project are the deprecated GTT parameter, the NPU/IOMMU failure, and suspect Thunderbolt GRO. There were no amdgpu reset/RAS/thermal or NVMe integrity errors in the inspected boot.

## USB4 and network deep audit

### Physical/router state

| Rail | Linux interface/address | Remote router | Link | MTU | Driver |
|---|---|---|---|---:|---|
| domain0 | thunderbolt0 / 10.44.0.1/30 | Linux nimo-2 | 2 x 20.0 Gb/s RX and TX | 9000 | thunderbolt-net |
| domain1 | thunderbolt1 / 10.44.0.5/30 | Linux nimo-2 | 2 x 20.0 Gb/s RX and TX | 9000 | thunderbolt-net |

Both host-router sysfs objects report generation=4 and are active. That router-generation label does not mean an 80 Gb/s USB4 Gen-4 data link: the live negotiated rate is explicitly two 20 Gb/s lanes, 40 Gb/s nominal per rail. Each path includes two retimers. Domain security is user and routers are authorized. Host-router power control is on; the remote routers had reverted to auto after enumeration but remained active. The sysfs speed/lane values and ethtool 40,000 Mb/s values agree.

Historical interface counters are large enough to demonstrate real traffic. thunderbolt0 had about 370 GB RX and 665 GB TX; thunderbolt1 had about 705 GB RX and 1.00 TB TX. Both report zero RX/TX errors and nine TX drops. MPTCP historical counters show no fallback, blackhole, checksum error, or stale subflow; only 34 MPTCP retransmits and 39 duplicate-data events over the boot history inspected.

### Current unloaded network state

- net.mptcp.enabled=1, but limits are add_addr_accepted=0 and subflows=0; no endpoints or MPTCP sockets exist. This is correct for the disabled/unloaded service.
- The preserved llm-dual-usb4-mptcp unit sets two subflows, signals 10.44.0.5 on thunderbolt1, and is required by the MiniMax service.
- The MiniMax launcher uses mptcpize, RPC endpoint 10.44.0.2:50052, layer split, and tensor split 1,1.
- Both rails use fq_codel with quantum 1526 despite MTU 9000. Cumulative requeues are high (approximately 0.82 million and 1.53 million), without qdisc drops.
- TCP uses cubic. busy_poll and busy_read are 100 microseconds. tcp_fastopen=3. Per-rail rp_filter is loose mode 2.
- rmem_max and wmem_max are 4 MiB; TCP autotuning permits 32 MiB receive and 4 MiB transmit. The short USB4 RTT makes these adequate for current bandwidth-delay product, but they should still be measured for large messages.

### Queue, offload, and IRQ state

- One RX and one TX queue per rail.
- RX checksum is fixed off; TX checksum, scatter/gather, TSO, GSO, and GRO are on.
- LRO, RX hashing/RSS, hardware GRO, and UDP GRO forwarding are off/fixed.
- The driver exposes no adjustable channels, rings, coalescing, private flags, or statistics.
- Global rps_sock_flow_entries is 0. Each rail has rps_cpus=0, rps_flow_cnt=0, xps_cpus=0.
- irqbalance is not installed and its unit is masked.
- Active data vectors for c7:00.5 are effective on CPUs 29 and 30; c7:00.6 data vectors are effective on CPUs 31 and 0. Control vectors land on CPUs 21-24. Their configured masks remain 0-31, so this is kernel-managed placement rather than an explicit cluster policy.

### Direct-stream readiness

The running 7.1.3 kernel cannot expose upstream USB4STREAM:

- CONFIG_USB4=m and CONFIG_USB4_NET=m are present, but its source/config has no USB4_CONFIGFS or USB4_STREAM option.
- Installed modules include thunderbolt.ko and thunderbolt_net.ko, but no thunderbolt_stream module.
- There is no USB4 stream ConfigFS tree and no /dev/tbstreamX character device.
- CONFIG_USB4_DMA_TEST is an unrelated loopback DMA test driver; its disabled state is not a USB4STREAM gate.

The upstream interface must be modeled accurately before designing the optional transport. Named streams are configured through ConfigFS and appear as /dev/tbstreamX. Userspace receives read/write/poll semantics; the implementation does not expose mmap or splice, mandates end-to-end flow control, limits DATA frames to 4 KiB, and copies between userspace iterators and page-backed frames. Its internal DMA rings are not a userspace bulk-ring ABI. Separate named control and data streams can coexist with USB4NET, but fewer copies, large aligned writes, and batching are hypotheses to measure. First test a supported kernel carrying CONFIG_USB4_STREAM; consider an isolated backport only after the reliable network baseline is preserved.

## GPU software stack

| Layer | Live evidence |
|---|---|
| Kernel amdgpu | 7.1.3 CachyOS module; gfx1151; HMM, DEVICE_PRIVATE, and ZONE_DEVICE compiled |
| ROCm | 7.2.4 packages; HIP 7.2.53211; AMD LLVM 22.0.0git |
| Libraries | rocBLAS, hipBLAS/hipBLASLt, rocWMMA, composable-kernel, RCCL, MIOpen all 7.2.4 generation |
| HIP target | gfx1151, 40 CU, wavefront 32, 2.9 GHz, base HSA profile |
| Vulkan | Mesa RADV 26.1.4, Vulkan 1.4; only radeon_icd.json present |
| Management | rocm-smi present but partially broken; amd-smi absent |

The live GPU performance level is high. SCLK is 2.9 GHz, MCLK 1.0 GHz, FCLK 2.0 GHz, and SOCCLK 1.472 GHz at idle. amdgpu no_system_mem_limit is Y even though the model is unloaded. The render node and /dev/kfd are mode 0666 under the distribution's default udev rules; this is normal for this installation but broadens access on a multi-user host.

## llama.cpp and model runtime

- Runtime path: /opt/llm-usb4-cluster/llama.
- Build commit: 8f114a9b573b69035299f9b924047f53c1e22c7e.
- Reported build: version 1 (8f114a9b), GNU 16.1.1 host compiler; libggml-hip uses AMD LLD/Clang 22.
- The retained canonical recipe at 02_build/output/remote/scripts/50-build-llamacpp-rocm.sh records Ninja/Release/shared plus GGML_HIP=ON, GGML_RPC=ON, GGML_HIP_ROCWMMA_FATTN=ON, and AMDGPU_TARGETS=gfx1151. The build log confirms HIP+RPC and ggml 0.16.0. The binary package contains libggml-hip.so and libggml-rpc.so but no Vulkan backend.
- llama-server and llama-cli report the commit. llama-bench and ggml-rpc-server lack a --version option; their attempted version probes exited after usage and did not run a benchmark.
- The recipe is recorded, but no source repository, generated CMake cache, complete compiler/dependency manifest, or proven rebuild remains under /opt. Binary reproduction is therefore unproven rather than wholly undocumented.
- Preserved MiniMax model: six Q6_K_XL shards, exactly 207,445,443,072 bytes (about 194 GiB). It exceeds one node's capacity and is suitable for layer-mode validation, not replica validation.
- Current environment: 131,072 total context, batch 4096, ubatch 4096, two parallel slots, q8_0 K/V cache, 16 CPU threads, polling/priority 2, direct I/O, no mmap, layer split across local and 10.44.0.2 RPC.
- The service binds 0.0.0.0:8081 and has metrics enabled. It is currently disabled/inactive and port 8081 is closed.
- A legacy Step-3.7 llama unit remains installed and inactive, but its referenced Vulkan executable and GGUF paths are missing; it is a stale definition, not an available Vulkan rollback. The inactive vLLM/Step-3.7 definitions remain, and three prior VLLM EngineCore crashes lack cores.

## Memory, UMA, and storage policy

- Linux MemTotal: 130,491,708 KiB; audit MemAvailable: approximately 122 GiB.
- Swap: /swapfile, 32 GiB, about 21 MiB used.
- zswap: enabled, zstd, maximum pool 20%, accept threshold 90%.
- THP: always; defrag defer+madvise. No 2 MiB or 1 GiB hugetlb pages are reserved.
- KSM is off. MGLRU is enabled. Automatic NUMA balancing is off, which is largely irrelevant on the single-node topology.
- swappiness=1, page-cluster=0, vfs_cache_pressure=50, max_map_count=1,048,576, compaction_proactiveness=0, overcommit_memory=0.
- cgroup v2 is active with memory/dmem controllers, but the user slice and inference units have no memory cap.
- systemd-oomd is masked. The MiniMax unit has LimitMEMLOCK=infinity; an ordinary SSH shell has only 8 MiB memlock.
- Root is Btrfs with noatime, zstd:1, SSD optimization, and async discard. About 239 GiB was free/estimated.
- NVMe scheduler is kyber, nr_requests 256, read-ahead 256 KiB, rq_affinity 1, and writeback throttling target 2 ms.

The 124 GiB GTT configuration is intentional for UMA inference and works. The audit does not prove that THP=always, zswap, kyber, or the 32 GiB swapfile are optimal. They should remain benchmark variables, not assumptions.

## Power, thermal, and background load

- amd-pstate-epp is active on every logical CPU.
- All 32 policies use governor performance and EPP performance; scaling range is 2.0-5.1875 GHz.
- processor.max_cstate=2 is a boot constraint and all CPU resume-latency QoS values are 100 microseconds.
- GPU is forced high and cannot downclock while idle.
- power-profiles-daemon is disabled; tuned is absent; TLP, auto-cpufreq, thermald, irqbalance, and systemd-oomd are masked/absent.
- Default target is multi-user, not graphical. Running services are lean: NetworkManager, SSH, boltd, Avahi, resolved, timesyncd, journald, logind, udev, D-Bus, and getty.
- No failed systemd unit remained at audit time. Normal maintenance timers (man-db, tmpfiles, shadow, logrotate, plocate, keyring sync) can still add noise during long benchmark runs.

The host had no thermal problem. The issue is policy leakage: it consumes idle power and removes the ability to compare on-demand versus pinned performance objectively.

## Security and operational constraints

Positive controls:

- SSH is key-only: PasswordAuthentication=no, KbdInteractiveAuthentication=no, PermitRootLogin=no, X11Forwarding=no.
- API key file mode is 0600 root:root; NetworkManager connection files are 0600.
- No current API/RPC listener exists.
- Kernel perf_event_paranoid=2, kptr_restrict=2, dmesg_restrict=1, ptrace_scope=1, unprivileged BPF restricted, ASLR enabled.

Constraints/gaps:

- UFW default incoming is allow; this is the most important security issue before service start.
- Secure Boot/lockdown and AppArmor/SELinux are absent. This may be acceptable for a trusted dedicated appliance but should be explicit.
- connorb has passwordless sudo.
- USB4 domain security is user, but the peer is authorized and private-rail UFW rules allow all traffic from the matching peer address to the matching local rail address.
- auditd is disabled. Coredump socket exists but crash storage was none, so prior VLLM failures are not diagnosable.
- IOMMU is off, reducing device isolation.

## Benchmark and profiling capability

Present:

- llama-bench and llama-batched-bench
- hipcc, hipconfig, rocminfo, rocm-smi, clinfo
- vulkaninfo, vkcube, glxinfo
- iperf3 3.21 with affinity, zerocopy, bind-to-device, GSO/GRO support
- bpftool
- numactl/numastat, hwloc/lstopo
- cpupower, lm_sensors, smartctl
- ethtool and iproute2 tools (ss, tc, nstat)
- podman

Absent and needed:

- perf
- rocprof/rocprofv3, rocprof-compute/Omniperf, rocgdb
- amd-smi and radeontop
- bpftrace and trace-cmd
- fio, stress-ng, sysbench, a confirmed STREAM memory benchmark
- sysstat tools (iostat, mpstat, sar, pidstat)
- nvme-cli
- sockperf, netperf, qperf or an equivalent small-message latency harness

## Change classes

### Low-risk, no-reboot candidates

These are recommendations only; none were applied.

1. Design and pin the missing observability/tool manifest without installing it yet.
2. Create the read-only collector source/specification for TTFT, decode/aggregate tokens per second, p50/p99, bytes/token, per-rail counters, IRQ/softirq CPU, clocks, power, temperature, GTT/RSS, and PSI.
3. Prepare the fork source, patch queue, build manifest, benchmark schema, and rollback pointers outside the known-good runtime path.
4. Define lifecycle state capture/restore and crash-telemetry behavior as reviewed unit changes before deploying them.

### Reversible but workload-disruptive tests

Run only while inference is stopped and SSH recovery is available:

1. Install the approved/pinned tool packages and run a smoke test; package transactions can alter dependencies.
2. Deploy lifecycle hooks only with unit backups, reload validation, and a known-good MiniMax/RPC smoke test.
3. Enable bounded crash telemetry for a controlled reproduction, with storage and secret-handling limits.
4. Archive stale unit backups only after path/rollback inventory and approval.
5. GRO on/off on each rail; GSO/TSO combinations where supported.
6. fq_codel versus fq/noqueue and MTU 9000 versus 1500.
7. Explicit Thunderbolt IRQ placement, reserved rail cores, process/HTTP affinity, and small targeted RPS masks.
8. zswap on/off or alternate compressor, THP always versus madvise, and hugetlb experiments.
9. NVMe kyber versus none for the non-mmap/direct-I/O loader.
10. Restore GPU auto and CPU powersave/EPP balance at idle, then apply high/performance only around a run.
11. Converge/tighten firewall policy only after explicit rules are staged; validate from a second session or console because an error can sever SSH.

### Reboot, firmware, or kernel-risk changes

1. Enable IOMMU and compare ROCm/HSA mapping, NPU enumeration, USB4, and performance.
2. Remove deprecated amdgpu.gttsize while retaining the proven TTM limits; verify exact 126,976 MiB GTT after reboot.
3. Test processor.max_cstate and pcie_aspm independently, not as a bundle.
4. Change BIOS performance/UMA/firmware settings only with a recorded before/after matrix and recovery plan.
5. Test a kernel carrying CONFIG_USB4_STREAM, or build an isolated backport only if justified. Keep USB4NET intact as recovery and control plane.

## Required test matrix

### Transport phase 1: USB4NET

For each rail independently and both rails concurrently:

- Payload sizes: 64 B, 256 B, 1 KiB, 4 KiB, 16 KiB, 64 KiB, 256 KiB, 1 MiB, 4 MiB, and at least 32 MiB.
- Measure one-way bandwidth, ping-pong RTT p50/p95/p99/max, syscall rate, CPU cycles/instructions, softirq time, context switches, queue requeues/drops, retransmits, and watts.
- Compare GRO on/off, TSO/GSO supported combinations, MTU 1500/9000, qdisc choices, send/receive buffer sizes, blocking versus busy-poll, and one versus multiple sockets.
- Compare default IRQ placement, dedicated physical cores, process affinity away from rail cores, and targeted RPS.
- For MPTCP, prove two subflows with per-subflow byte counts, balanced use, failover, reconnect behavior, and absence of LAN fallback.

### Transport phase 2: direct stream

- On a supported kernel, configure separate named control and data streams through ConfigFS and validate /dev/tbstreamX read/write/poll behavior, mandatory E2E flow control, <=4 KiB DATA framing, reconnect/reset, integrity, and timeout semantics.
- The upstream ABI copies through page-backed frames and exposes no mmap/splice. Measure large aligned userspace writes, write coalescing into 4 KiB frames, pinned versus pageable application staging, one/two named streams, one/two rails, CPU consumption, and copy count without assuming zero-copy or a userspace bulk ring.
- Gate success on both throughput and small-message tail latency; a raw bandwidth win that increases decode p99 is not sufficient.

### GPU/kernel work

- Build HIP and Vulkan from the same llama.cpp commit and flags.
- Dense and MoE models; Q4_K, Q5_K, Q6_K, Q8_0; short/long contexts; batch 1, prefill batches, and multiple concurrent requests.
- Profile mul_mat_vec_q, quantize_q8_1, dequant/GEMV, RMSNorm, RoPE, gating/SiLU, reductions, launch count, occupancy, VGPRs, memory bandwidth, and cache behavior.
- Autotune Flash Attention by shape/context/backend; do not enable globally.
- Record TTFT, prompt tokens/s, decode tokens/s, aggregate tokens/s, p50/p99, power, temperature, and GTT/RSS for every result.

### Distributed modes

1. Replica/data parallel prerequisite: choose a separate model that fits each node; the preserved 207,445,443,072-byte MiniMax artifact does not. Require 1.7-1.9x aggregate throughput under sufficient concurrency, stable session/slot/KV affinity, and zero normal cross-node model/KV traffic.
2. Layer pipeline for MiniMax: contiguous ownership, one major activation boundary, KV remains with its layers, persistent graph upload once with command reuse, microbatch interleaving, activation bytes/token, stage idle time, and clean failure/reconnect behavior.
3. Speculative draft/verify: transfer only token/proposal/acceptance messages, keep independent local KV state, transfer no KV or hidden tensors, report acceptance/verifier batch/bytes per output token, and require a declared positive single-stream latency or tokens/s gain.
4. Tensor/row split is deferred research, not a required initial acceptance test. Revisit only after the three target modes and only with an explicit collective/reduction cost case.

Do not add normal KV migration or prefill/decode disaggregation to any acceptance path.

### Memory/load stability

- Cold and warm model loading under each THP/zswap/NVMe policy.
- Sustained 1, 2, 4, and 8 stream runs with PSI, swap-in/out, GTT/RSS, clocks, thermals, errors, and GPU reset monitoring.
- Clean start/stop/restart cycles proving that all temporary tuning is restored.

## Unknowns and evidence limits

- BIOS Performance mode and fixed UMA framebuffer selection are not exposed through Linux platform_profile. Prior project evidence says Performance/Auto, but this audit could not independently read the setup variable.
- Firmware 3.05 update availability and release notes were not checked; fwupd is masked and no vendor lookup was performed.
- The exact cable/physical-port labels cannot be derived from the logical domain0/domain1 names alone.
- GRO's real effect remains unknown until controlled A/B tests; the kernel warning is a profiling lead, not proof of a throughput regression.
- Upstream USB4STREAM semantics are known, but supported-kernel versioning, distro availability, backport feasibility, performance, and maintenance burden remain unknown; the installed kernel definitively does not expose it.
- Sustained power ceiling, throttling, HIP versus Vulkan speed, and current model throughput were intentionally not measured while unloaded.
- The core build recipe/flags and commit are retained, but generated cache values, complete toolchain/dependency state, gfx1151 code-object inventory, and a proven rebuild are absent.
- USB4NET lacks ethtool driver statistics, so packet/copy internals require tracing or a patched driver.
- The retained 32 GiB swapfile and zswap policy were not stress-tested in this audit.

## Exact command record

Commands were invoked from the shared Windows workstation. Multi-line groups were piped as LF-normalized shell text to ssh -o BatchMode=yes nimo-1 bash -s. Output was filtered to avoid machine IDs, serial numbers, UUIDs, credentials, tokens, private keys, and API-key contents.

Identity and baseline:

    ssh -G nimo-1
    ssh -o BatchMode=yes -o ConnectTimeout=8 nimo-1 bash -s
    hostname; hostname -f; id; uname -srmo
    . /etc/os-release; date --iso-8601=seconds
    sha256sum /proc/sys/kernel/random/boot_id /etc/machine-id
    ssh-keygen -lf /etc/ssh/ssh_host_ed25519_key.pub
    ssh-keygen -F nimo-1.local
    ip -brief address show up; ip -o route show default
    sudo -n true

Hardware, firmware, kernel, and storage:

    hostnamectl
    sudo -n dmidecode -s bios-vendor
    sudo -n dmidecode -s bios-version
    sudo -n dmidecode -s bios-release-date
    sudo -n dmidecode -s system-manufacturer
    sudo -n dmidecode -s system-product-name
    sudo -n dmidecode -t 16
    sudo -n dmidecode -t 17
    lscpu; lscpu -e=CPU,NODE,SOCKET,CORE,ONLINE,MAXMHZ,MINMHZ
    lspci -tv; lspci -nnk; lspci -s c5:00.0 -nnvv
    numactl -H; free -h; lsmem; lstopo-no-graphics --of console
    cat /proc/cmdline; cat /proc/version
    zgrep -E '^CONFIG_(PREEMPT|PREEMPT_DYNAMIC|NO_HZ|AMD_PSTATE|HMM_MIRROR|DEVICE_PRIVATE|ZONE_DEVICE|DRM_AMDGPU|MPTCP|MPTCP_IPV6|USB4|AMD_IOMMU|IOMMU_SUPPORT|TRANSPARENT_HUGEPAGE|HUGETLBFS|PERF_EVENTS|BPF|BPF_SYSCALL|KEXEC|KSM)=' /proc/config.gz
    bootctl status --no-pager
    cat /sys/kernel/security/lockdown
    cat /sys/firmware/acpi/platform_profile
    pacman -Q amd-ucode linux-cachyos linux-firmware
    fwupdmgr get-devices
    sudo -n journalctl -k -b --no-pager
    sudo -n journalctl -k -b -p warning..alert --no-pager
    lsblk -e7 -o NAME,KNAME,TYPE,SIZE,ROTA,TRAN,MODEL,FSTYPE,FSVER,MOUNTPOINTS,FSAVAIL,FSUSE%,DISC-GRAN,DISC-MAX
    findmnt -rn -o TARGET,SOURCE,FSTYPE,OPTIONS
    sudo -n smartctl -a /dev/nvme0
    swapon --show; btrfs filesystem usage -T /; btrfs device stats /

GPU/software:

    cat /sys/class/drm/card0/device/mem_info_*
    cat /sys/class/drm/card0/device/pp_dpm_sclk
    cat /sys/class/drm/card0/device/pp_dpm_mclk
    cat /sys/class/drm/card0/device/pp_dpm_fclk
    cat /sys/class/drm/card0/device/pp_dpm_socclk
    cat /sys/class/drm/card0/device/power_dpm_force_performance_level
    sudo -n cat /sys/kernel/debug/dri/0/amdgpu_pm_info
    modinfo amdgpu
    sudo -n cat /sys/module/amdgpu/parameters/*
    cat /sys/class/kfd/kfd/topology/nodes/*/properties
    pacman -Q | grep -Ei '^(rocm|hip|hsa|rocblas|hipblas|hipblaslt|rocwmma|miopen|composable|amd-smi|roctracer|rocprof|rocrand|rccl|mesa|vulkan|spirv|opencl|llvm-amdgpu)'
    hipconfig --full; rocminfo
    rocm-smi --showproductname --showdriverversion --showvbios --showuse --showmemuse --showmeminfo vram --showmeminfo gtt --showclocks --showtemp --showpower --showpids
    vulkaninfo --summary
    ldconfig -p
    /opt/llm-usb4-cluster/llama/llama-server --version
    /opt/llm-usb4-cluster/llama/llama-cli --version
    /opt/llm-usb4-cluster/llama/llama-bench --version
    /opt/llm-usb4-cluster/llama/ggml-rpc-server --version
    ldd /opt/llm-usb4-cluster/llama/llama-server
    readelf -p .comment /opt/llm-usb4-cluster/llama/libggml-hip.so

USB4/network:

    lsmod; modinfo thunderbolt; modinfo thunderbolt_net
    find /sys/bus/thunderbolt/drivers -maxdepth 2
    find /sys/bus/thunderbolt/devices -maxdepth 4
    for d in /sys/bus/thunderbolt/devices/*; do for f in device_name vendor_name authorized generation security nvm_version usb4_version rx_speed tx_speed rx_lanes tx_lanes link_speed lanes route key device vendor; do test -r "$d/$f" && printf '%s=%s\n' "$f" "$(cat "$d/$f")"; done; done
    find /sys/kernel/debug/thunderbolt -maxdepth 3 -type f
    sudo -n cat /sys/kernel/debug/thunderbolt/0-0/port*/counters
    sudo -n cat /sys/kernel/debug/thunderbolt/1-0/port*/counters
    sudo -n lspci -s c7:00.5 -vv
    sudo -n lspci -s c7:00.6 -vv
    ip -details -statistics link show
    ip -brief addr; ip route show table all; ip rule show
    ip mptcp limits show; ip mptcp endpoint show; ss -Mni
    nmcli device; nmcli connection show --active
    tc -s qdisc show; tc -s class show
    ethtool thunderbolt0; ethtool thunderbolt1
    ethtool -i -k -l -g -c -S thunderbolt0
    ethtool -i -k -l -g -c -S thunderbolt1
    cat /sys/class/net/thunderbolt*/queues/*/rps_cpus
    cat /sys/class/net/thunderbolt*/queues/*/rps_flow_cnt
    cat /sys/class/net/thunderbolt*/queues/*/xps_cpus
    cat /proc/sys/net/core/rps_sock_flow_entries
    cat /proc/interrupts
    for irq in 84 86 88 90 103 104 105 106; do cat /proc/irq/$irq/smp_affinity_list /proc/irq/$irq/effective_affinity_list; done
    systemctl show irqbalance.service
    for k in net.core.busy_poll net.core.busy_read net.core.rmem_max net.core.wmem_max net.core.netdev_max_backlog net.core.default_qdisc net.ipv4.tcp_congestion_control net.ipv4.tcp_rmem net.ipv4.tcp_wmem net.ipv4.tcp_fastopen net.ipv4.tcp_low_latency net.ipv4.conf.thunderbolt0.rp_filter net.ipv4.conf.thunderbolt1.rp_filter net.mptcp.enabled net.mptcp.checksum_enabled net.mptcp.scheduler net.mptcp.pm_type; do sysctl "$k"; done
    ss -s; nstat -az
    find /lib/modules/$(uname -r) -type f | grep thunderbolt/usb4
    zgrep -E '^CONFIG_USB4(_CONFIGFS|_STREAM|_DMA_TEST)?=' /proc/config.gz
    grep -n -A18 -B5 -E 'config USB4_(CONFIGFS|STREAM)' /usr/src/linux-cachyos/drivers/thunderbolt/Kconfig
    find /lib/modules/$(uname -r) -type f -iname '*thunderbolt*stream*' -o -iname '*tbstream*'
    find /dev -maxdepth 1 -iname 'tbstream*' -ls
    ls -ld /sys/kernel/config/usb4 /sys/kernel/config/usb4/subsystems

Memory, power, services, and security:

    grep -E '^(MemTotal|MemFree|MemAvailable|Buffers|Cached|SwapTotal|SwapFree|AnonPages|Mapped|Shmem|Slab|SReclaimable|PageTables|KernelStack|Committed_AS|CommitLimit|VmallocUsed|Percpu|HardwareCorrupted|AnonHugePages|ShmemHugePages|FileHugePages|HugePages_|Hugepagesize|Hugetlb):' /proc/meminfo
    cat /proc/pressure/memory /proc/pressure/cpu /proc/pressure/io
    cat /sys/kernel/mm/transparent_hugepage/*
    cat /sys/kernel/mm/transparent_hugepage/khugepaged/*
    cat /sys/devices/system/node/node*/hugepages/*/*
    cat /sys/kernel/mm/ksm/*; cat /sys/kernel/mm/lru_gen/*
    for k in vm.swappiness vm.page-cluster vm.vfs_cache_pressure vm.dirty_background_bytes vm.dirty_bytes vm.overcommit_memory vm.overcommit_ratio vm.max_map_count vm.min_free_kbytes vm.watermark_scale_factor vm.zone_reclaim_mode vm.compaction_proactiveness vm.nr_hugepages vm.nr_overcommit_hugepages kernel.numa_balancing; do sysctl "$k"; done
    cat /proc/buddyinfo; cat /proc/zoneinfo
    cat /sys/devices/system/cpu/cpufreq/policy*/*
    cpupower frequency-info
    cat /sys/devices/system/cpu/cpu0/cpuidle/state*/*
    sensors; vmstat 1 3
    ps -eo pid,ppid,user,ni,psr,pcpu,pmem,rss,stat,comm
    systemctl --failed
    systemctl list-units --type=service --state=running
    systemctl list-timers --all
    for u in minimax-m27-q6-server.service llm-dual-usb4-mptcp.service llm-usb4-pmqos.service step37-performance-tuning.service step37-container.service step37-ray-head.service step37-vllm.service llama-step37.service irqbalance.service systemd-oomd.service; do systemctl show "$u" --no-pager; done
    systemctl --user list-unit-files; systemctl --user list-units --all
    ss -lntup
    podman ps -a; podman images
    sudo -n ufw status verbose; sudo -n nft -nn list ruleset
    sudo -n sshd -T
    sysctl kernel.perf_event_paranoid kernel.kptr_restrict kernel.dmesg_restrict kernel.yama.ptrace_scope kernel.unprivileged_bpf_disabled
    ls -l /dev/kfd /dev/dri/* /dev/tpm*
    coredumpctl list; for pid in 9289 40392 2381; do coredumpctl info "$pid" --no-pager; done
    for c in perf bpftool bpftrace trace-cmd rocprof rocprofv3 rocprof-compute omniperf rocgdb rocm-smi amd-smi radeontop clinfo hipcc hipconfig rocminfo vulkaninfo vkcube glxinfo iperf3 netperf qperf sockperf nuttcp ntttcp fio stress-ng sysbench mbw tinymembench stream numactl numastat lstopo-no-graphics hwloc-ls likwid-perfctr turbostat cpupower sensors smartctl nvme iostat mpstat sar pidstat ethtool tc ss nstat mptcpize podman ray vllm; do command -v "$c" || true; done

## Bottom line

nimo-1's hardware and physical USB4 topology are ready for serious work; the current limitation is software architecture and observability, not cable training or basic OS setup. Use replica/data parallel as the default for a separate model that fits each node; use one-boundary layer mode for the preserved 207 GB MiniMax artifact; use speculative draft/verify as the single-stream experiment. Before kernel work, make the firewall a service-start gate, make tuning lifecycle-scoped, approve/install profilers, and establish a reproducible HIP/Vulkan/transport baseline. Optimize USB4NET only against the already strong measured baseline. Treat upstream USB4STREAM as an optional later supported-kernel experiment, and keep all scheduler, transport, and gfx1151 changes modular for rebasing/upstreaming.
