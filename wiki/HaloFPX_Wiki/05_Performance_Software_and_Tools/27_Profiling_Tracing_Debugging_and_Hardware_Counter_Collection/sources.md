---
section_id: "27"
title: "Profiling sources"
status: "draft"
last_verified: "2026-07-16"
applies_to:
  repositories: ["charlie12345/ROCmFPX", "ggml-org/llama.cpp"]
  software_versions: ["ROCprofiler-SDK 1.3.2 documentation", "Linux tracing interfaces"]
  hardware_revisions: ["gfx1151 Strix Halo", "dual USB4 host links"]
related_sections: ["24", "25", "26", "28"]
---

# Profiling sources

Access date: 2026-07-16.

| ID | Primary source and revision | Supports | Limitations |
|---|---|---|---|
| S27-01 | AMD, [rocprofv3 CLI options](https://rocm.docs.amd.com/projects/rocprofiler-sdk/en/develop/quick-reference/rocprofv3-cli-options.html), ROCprofiler-SDK 1.3.2 docs | Trace domains, KFD events, output config, counters | `develop` is volatile; pin installed component docs. |
| S27-02 | AMD, [Using rocprofv3](https://rocm.docs.amd.com/projects/rocprofiler-sdk/en/develop/how-to/using-rocprofv3.html), ROCprofiler-SDK 1.3.2 docs | Counter types, multi-pass behavior, output formats | Examples do not prove gfx1151 availability. |
| S27-03 | AMD GPUOpen, [Radeon GPU Profiler](https://gpuopen.com/rgp/), accessed 2026-07-16 | RGP purpose and current distribution | Marketing/landing page; compatibility must be checked. |
| S27-04 | AMD GPUOpen, [Radeon Developer Panel manual](https://gpuopen.com/manuals/rdp_manual/), accessed 2026-07-16 | Linux Vulkan capture compatibility warning | Driver-version-specific and volatile. |
| S27-05 | Linux kernel, [perf security and interface](https://docs.kernel.org/admin-guide/perf-security.html), current docs | perf access model and capabilities | Actual PMU events are machine-specific. |
| S27-06 | Linux kernel, [ftrace](https://docs.kernel.org/trace/ftrace.html), current docs | Tracers, tracefs buffers, block/scheduler/latency tracing | Kernel config and tracepoints vary. |
| S27-07 | Linux man-pages, [iostat(1)](https://man7.org/linux/man-pages/man1/iostat.1.html), current man page | Extended CPU/device statistics | Aggregated intervals can hide short stalls. |
| S27-08 | NVM Express, [nvme-cli](https://github.com/linux-nvme/nvme-cli), repository documentation accessed 2026-07-16 | NVMe log/identify tooling | Device support and permissions vary. |
| S27-09 | systemd, [systemd-coredump](https://www.freedesktop.org/software/systemd/man/latest/systemd-coredump.html), current manual | Core capture/storage model | Local configuration governs retention. |
| S27-10 | Linux PTP Project, [linuxptp documentation](https://www.linuxptp.org/documentation/), accessed 2026-07-16 | PTP tools and clock synchronization | Accuracy is topology/hardware dependent. |
| S27-11 | Linux kernel, [BPF documentation](https://docs.kernel.org/bpf/), current docs | Kernel BPF facilities | Program safety and overhead require validation. |
| S27-12 | Linux kernel, [Pressure Stall Information](https://docs.kernel.org/accounting/psi.html), current docs | CPU/memory/I/O pressure signals | Pressure is not direct device latency. |

## Freshness warning

**[OPEN]** ROCprofiler `develop`, GPUOpen manuals and kernel `latest` pages can change. The accepted experiment environment must archive or link the exact installed-source revision before claims are promoted.

