# Observability and Telemetry

> **Wiki status:** Proposed · **Evidence state:** D0 — design only · **Last reviewed:** 2026-07-17  
> **Machine-validation status:** Not run. Missing measurements are `INSUFFICIENT_EVIDENCE`, never an implicit pass.


## Sampling cadence

- Token events: every emitted non-empty token at client; server timestamps when available.
- Host, GPU, link, and disk telemetry: 1 Hz for normal tests; 5–10 Hz for short microbench/fault windows if collector overhead remains below 1% CPU and 0.5% throughput.
- Power meter: native rate, minimum 1 Hz.
- Kernel/journal events: continuous.
- USB4 topology and negotiated state: before, after, and on udev/domain events.

## Required channels per node

| Domain | Minimum signals |
|---|---|
| CPU | per-core utilization/frequency, process CPU, load/run queue, context switches |
| Memory | MemAvailable, RSS/PSS, major/minor faults, swap, PSI, cgroup memory events |
| GPU/APU | GPU/memory busy, clocks, memory use, power, temperature, throttler, fan if exposed |
| Disk | cgroup `io.stat`, device bytes/IOPS/latency, filesystem free space, model file faults |
| Network | interface bytes/packets/errors/drops, TCP retransmits, RTT/cwnd snapshots, MTU/offloads |
| USB4 | domain/device IDs, generation, RX/TX speed and lane count, reconnect/authorization events |
| Runtime | queue depth, active/waiting requests, cached/processed tokens, prompt/decode timings, errors |
| Logs | engine stdout/stderr, system journal, kernel ring buffer, service manager status |

Linux documents USB4NET host-to-host operation and USB4 sysfs speed/lane attributes. AMDGPU exposes `gpu_busy_percent`, `mem_busy_percent`, and `gpu_metrics`; ROCm 7.14 documentation also identifies profiling support for Strix/Halo-class APUs. [[SRC-010]](../references/Sources.md#src-010) [[SRC-012]](../references/Sources.md#src-012) [[SRC-013]](../references/Sources.md#src-013) [[SRC-014]](../references/Sources.md#src-014)

## Clock rules

Use monotonic timestamps for durations and UTC timestamps for cross-file correlation. Capture monotonic-to-UTC anchors at collector start/end. Client-observed TTFT and ITL must be computed on one clock and do not require cross-host synchronization. For cross-node causality, require measured time offset ≤5 ms and record the method.

## Collector overhead

Run one no-telemetry control for each collector profile. If median throughput changes by >1% or p95 latency by >2%, reduce sampling or use a lower-overhead source. The collector version and command line are provenance fields.
