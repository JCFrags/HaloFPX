---
section_id: "75"
title: "Fabric Benchmark Facts and Constraints"
status: "needs-machine-validation"
last_verified: "2026-07-17"
applies_to:
  repositories: []
  software_versions: ["iperf3 3.21", "HIP 6.4.x", "Linux 6.15 documentation"]
  hardware_revisions: ["planned dual Strix Halo nodes; exact USB4 topology open"]
related_sections: ["20", "49", "52", "54", "73", "76"]
---

# Facts and Constraints

## Metric contract

**[VERIFIED]** One-way delay is the interval from a source sending a packet to a destination receiving it. RFC 7679 requires accurate Type-P reporting, a stated late-versus-lost threshold, calibration results, and path information; clock and host timestamping uncertainty are part of the result. [S75-001]

**[VERIFIED]** IP packet delay variation is defined from differences between selected one-way delays. Sequence numbers are needed because loss, duplication, and reordering occur. A single variation sample is not statistically meaningful by itself. [S75-002]

**[VERIFIED]** RFC 9473 separates link capacity (maximum correctly receivable adjacent-link rate) from link usage (actual correctly received rate), and generalizes delay/loss vocabulary beyond IP. HaloFPX reports achieved goodput and never labels it physical capacity without appropriate evidence. [S75-003]

**[RECOMMENDATION]** For each request/transfer, record:

| Metric | Definition for this section |
|---|---|
| RTT | sender timestamp before enqueue to validated reply at sender |
| one-way delay | synchronized send-to-receive timestamps, with clock uncertainty |
| goodput | validated application payload bytes divided by timed interval |
| wire/transport throughput | transport-reported transferred bytes divided by interval |
| error rate | corrupt, lost, duplicate, reordered, timed-out, or retried units / attempted units |
| CPU cost | process and system CPU time plus cycles/instructions per validated GiB |
| interrupt cost | per-vector/per-CPU interrupt deltas per validated GiB |
| GPU end-to-end latency | producer completion through consumer validation completion, including required visibility barriers |

Percentiles must state estimator, population, sample count, and whether timeouts are censored. Never substitute the mean for p95/p99.

## Host transport tools

**[VERIFIED]** iperf3 3.21 supports JSON output, parallel streams, reverse direction, simultaneous bidirectional tests, UDP loss/jitter reporting, and a zero-copy option. Parallel streams can hide a CPU-limited single stream; therefore single- and multi-stream results answer different questions. [S75-005][S75-006]

**[VERIFIED]** RFC 6349 recommends establishing path MTU, baseline RTT, and bottleneck bandwidth before sustained TCP testing; sizing socket windows relative to bandwidth-delay product; and running each direction independently before simultaneous bidirectional tests. [S75-004]

**[INFERENCE]** iperf3 cannot by itself establish GPU-to-peer-GPU latency, zero-copy, correct cache visibility, or independence of two physical USB4 paths. Its endpoints and timing stop in the host networking stack.

## Linux observability

**[VERIFIED]** Linux exposes standard and driver-specific interface counters through netlink and ethtool, including per-queue or hardware-specific statistics when the driver implements them. An absent counter is not equivalent to zero. [S75-007]

**[VERIFIED]** Linux socket timestamping can expose software and hardware TX/RX timestamps; hardware support depends on the interface/PHY/driver and its PTP hardware clock. Timestamp location and conversion must be documented. [S75-008]

**[RECOMMENDATION]** Capture before/after deltas from `ip -s -j link`, `ethtool -S`, `/proc/interrupts`, `/proc/softirqs`, `nstat -az`, and transport-specific logs. Preserve CPU affinity, IRQ affinity, coalescing, MTU, qdisc, congestion control, socket buffers, and offload settings.

## HIP timing and visibility

**[VERIFIED]** In HIP coarse-grained memory becomes current across agents only after an appropriate synchronization such as device, stream, event, or blocking-copy synchronization. HIP asynchronous copies do not block the CPU, and events/stream waits express ordering. [S75-009][S75-010]

**[INFERENCE]** A benchmark that stops when the sender CPU enqueues work measures submission, not delivery or peer GPU consumption. The consumer must validate sequence, payload digest/pattern, and generation number after the specified synchronization.

## Scope limitations

**[ASSUMPTION]** The two USB4 cables can be bound to identifiable transport paths. Physical-controller and shared-bottleneck independence is not yet verified.

**[OPEN]** Whether peer GPU memory is directly transport-addressable, staged through pinned host memory, or copied through pageable memory depends on Sections 24, 50, 51, and 54 and must be recorded per run.

## Bounded reachability diagnostic

**[MEASURED]** On 2026-07-17, five ICMP echo requests per direction and rail produced zero loss [S75-L01]:

| Direction | Rail A mean RTT | Rail B mean RTT |
|---|---:|---:|
| nimo-1 → nimo-2 | 0.095 ms | 0.100 ms |
| nimo-2 → nimo-1 | 0.097 ms | 0.087 ms |

**[RECOMMENDATION]** Use these only as a preflight sanity range. They do not replace payload sweeps, sample-size/p99 reporting, controlled interface-counter deltas, simultaneous load, or GPU-to-peer-GPU validation.
