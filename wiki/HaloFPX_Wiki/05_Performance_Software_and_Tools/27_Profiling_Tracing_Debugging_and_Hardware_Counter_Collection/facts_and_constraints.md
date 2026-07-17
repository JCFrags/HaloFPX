---
section_id: "27"
title: "Profiling facts and tool matrix"
status: "needs-machine-validation"
last_verified: "2026-07-16"
applies_to:
  repositories: ["charlie12345/ROCmFPX", "ggml-org/llama.cpp"]
  software_versions: ["ROCprofiler-SDK 1.3.2 documentation", "Linux tracing interfaces"]
  hardware_revisions: ["gfx1151 Strix Halo", "dual USB4 host links"]
related_sections: ["24", "25", "26", "28"]
---

# Profiling facts and tool matrix

| Layer | Primary tools | Evidence-backed capability | Constraint |
|---|---|---|---|
| HIP/HSA/GPU | `rocprofv3`, `rocprofv3-avail`, ROCTx | **[VERIFIED]** Runtime/API, kernel, memory-copy, scratch, HSA and KFD traces; device counters; PFTrace/OTF2 and tabular outputs. [S27-01][S27-02] | Counters vary by GPU and incompatible sets require distinct full application passes. |
| Vulkan/RADV | Vulkan validation layers, `VK_KHR_performance_query` where exposed, Mesa debug/perf tooling, RenderDoc/RGP where supported | **[VERIFIED]** RGP is an AMD tool for low-level Radeon GPU analysis, but capture support depends on hardware/driver/API. [S27-03][S27-04] | **[OPEN]** Current Linux Developer Panel documentation warns newer driver releases may not capture Vulkan; validate exact Mesa/driver tuple. |
| CPU/process | `perf stat`, `perf record`, `perf sched`, FlameGraph | **[VERIFIED]** Linux perf exposes sampling, event counting and scheduler analysis subject to kernel permissions and PMU support. [S27-05] | Sampling and unwind mode affect overhead/fidelity. |
| Kernel/scheduler/IRQ | tracefs/ftrace, `trace-cmd`, eBPF/bpftrace | **[VERIFIED]** ftrace provides function, scheduler and latency tracers including `irqsoff`, `wakeup`, and block tracing. [S27-06] | Root/capabilities and kernel config commonly required; ring buffers can overrun. |
| USB4/PCIe/network | tracepoints, ftrace/eBPF, `ethtool -S`, `ip -s`, `ss`, driver debugfs | **[INFERENCE]** Correlating network and driver tracepoints can locate queueing and retransmission, but available USB4/thunderbolt events are kernel/driver specific. | Never assume a tracepoint exists; inventory first. |
| Filesystem/block/NVMe | `iostat -x`, `blktrace`/block tracepoints, `nvme smart-log`, `smartctl`, PSI | **[VERIFIED]** block tracing is integrated with ftrace and SMART/NVMe tools expose device telemetry. [S27-06][S27-07][S27-08] | Filesystem, block and device queues use different identifiers/timestamps; map device topology. |
| Crashes | systemd-coredump/coredumpctl, GDB, debuginfod/local symbols, kernel pstore/kdump | **[VERIFIED]** systemd-coredump stores and indexes userspace core metadata/core files according to configuration. [S27-09] | Secrets/model data may occur in cores; access and retention policy required. |

## Counter interpretation

- **[VERIFIED]** ROCprofiler distinguishes basic hardware counters from derived metrics, and counter availability is architecture-specific. [S27-02]
- **[RECOMMENDATION]** Save `rocprofv3-avail` output with the tool version and GPU agent identity before defining a metric set.
- **[INFERENCE]** Multi-pass counter data is comparable only if application input/state and GPU behavior are repeatable. Autoregressive scheduling, caches and thermals can break that assumption.
- **[RECOMMENDATION]** Treat profiler-derived occupancy/busy metrics as observations for that run, not end-to-end performance proof.

## Time domain constraint

**[VERIFIED]** PTP exposes clock synchronization and hardware/software timestamping mechanisms, but achievable accuracy depends on clock and network hardware. [S27-10] **[RECOMMENDATION]** Record each event's native clock ID, conversion method and estimated synchronization error. If hardware timestamping is unavailable, use round-trip protocol markers and report uncertainty; do not claim precise one-way latency.

