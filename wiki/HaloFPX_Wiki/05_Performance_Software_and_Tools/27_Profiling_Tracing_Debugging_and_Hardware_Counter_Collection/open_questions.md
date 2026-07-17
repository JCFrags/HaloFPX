---
section_id: "27"
title: "Profiling open questions"
status: "needs-machine-validation"
last_verified: "2026-07-16"
applies_to:
  repositories: ["charlie12345/ROCmFPX", "ggml-org/llama.cpp"]
  software_versions: ["ROCprofiler-SDK 1.3.2 documentation", "Linux tracing interfaces"]
  hardware_revisions: ["gfx1151 Strix Halo", "dual USB4 host links"]
related_sections: ["24", "25", "26", "28"]
---

# Profiling open questions

| ID | Question | Evidence needed | Blocks |
|---|---|---|---|
| O27-01 | **[OPEN]** Which rocprofv3 traces and counters are exposed for both gfx1151 agents? | `rocprofv3-avail` plus test captures | GPU metric plan |
| O27-02 | **[OPEN]** What capture overhead and event loss occurs at each trace level? | Profiled/unprofiled repeated trials and buffer stats | Trace validity |
| O27-03 | **[OPEN]** Can RGP capture the selected HIP or RADV path on the exact current driver? | Minimal capture with versions | RGP use |
| O27-04 | **[OPEN]** Which thunderbolt/USB4/PCIe and NIC tracepoints exist in the selected kernel? | tracefs inventory and driver source | Link diagnosis |
| O27-05 | **[OPEN]** Are both logical transport links independent at IRQ, queue and physical route levels? | Topology, counters and simultaneous load trace | Dual-link attribution |
| O27-06 | **[OPEN]** What cross-host clock error is achievable, and is hardware timestamping exposed? | PTP/chrony/timestamp inventory and calibration | One-way timing |
| O27-07 | **[OPEN]** Which PMU events can be collected without multiplexing on the selected CPU/kernel? | `perf list/stat` validation | CPU metric set |
| O27-08 | **[OPEN]** How do page-cache and direct-I/O paths appear for persistent KV operations? | Filesystem/block/NVMe correlated trace | Cache I/O tuning |
| O27-09 | **[OPEN]** What core-dump retention/redaction policy protects prompts, tokens and credentials? | Security review and fault-injection test | Crash pipeline |

## Internet follow-up

- Pin ROCprofiler-SDK docs/source to the installed 7.14 component revision.
- Review the selected kernel and USB4/network driver sources for tracepoint names rather than copying commands from another kernel.
- Verify RGP/Radeon Developer Panel compatibility notes for the exact Mesa/KMD release.

## Machine work

Execute every question on both nodes. Store raw captures under experiments/evidence with build and environment manifests. No hardware counter or latency claim is currently measured.

