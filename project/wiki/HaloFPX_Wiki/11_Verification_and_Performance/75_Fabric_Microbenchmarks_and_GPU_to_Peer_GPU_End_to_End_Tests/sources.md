---
section_id: "75"
title: "Fabric Benchmark Sources"
status: "needs-machine-validation"
last_verified: "2026-07-17"
applies_to:
  repositories: []
  software_versions: ["iperf3 3.21", "HIP 6.4.x", "Linux 6.15 documentation"]
  hardware_revisions: []
related_sections: ["49", "54", "73"]
---

# Sources

## S75-L01 — Live dual-rail reachability diagnostic

- Canonical source: [`../../../../sources/measurements/2026-07-17-strix-halo-live-inventory/README.md`](../../../../sources/measurements/2026-07-17-strix-halo-live-inventory/README.md)
- Capture: both directions on both private USB4NET rails, 2026-07-17.
- Supports: five-sample RTT summaries and zero observed loss for the bounded preflight.
- Limitations: ICMP, tiny sample, no payload sweep, no p95/p99, no controlled simultaneous load, and no GPU endpoint.

Accessed 2026-07-16. These sources define methodology and tool semantics; none reports HaloFPX results.

## S75-001 — RFC 7679: One-Way Delay

- **Publisher/revision:** IETF/RFC Editor, Internet Standard RFC 7679, January 2016.
- **URL:** https://www.rfc-editor.org/rfc/rfc7679.html
- **Supports:** one-way delay, Type-P, loss threshold, path, clock/calibration reporting.
- **Limit:** IP methodology, not a USB4 or GPU benchmark implementation.

## S75-002 — RFC 3393: IP Packet Delay Variation

- **Publisher/revision:** IETF/RFC Editor, RFC 3393, November 2002.
- **URL:** https://www.rfc-editor.org/rfc/rfc3393.html
- **Supports:** delay-variation definition, sequence handling, sampling limitations.
- **Limit:** one-way clocks and IP scope require adaptation.

## S75-003 — RFC 9473: Vocabulary of Path Properties

- **Publisher/revision:** IETF/RFC Editor, RFC 9473, November 2023.
- **URL:** https://www.rfc-editor.org/rfc/rfc9473.html
- **Supports:** capacity versus usage and non-IP path vocabulary.
- **Limit:** vocabulary, not an acceptance threshold.

## S75-004 — RFC 6349: TCP Throughput Testing

- **Publisher/revision:** IETF/RFC Editor, RFC 6349, August 2011.
- **URL:** https://www.rfc-editor.org/rfc/rfc6349.html
- **Supports:** path MTU, RTT, BDP/socket controls, uni/bidirectional sequence.
- **Limit:** informational TCP framework; not GPU E2E evidence.

## S75-005 — iperf3 documentation

- **Publisher/revision:** ESnet, iperf3 3.21 documentation, accessed 2026-07-16.
- **URL:** https://software.es.net/iperf/
- **Supports:** tool purpose, JSON and zero-copy availability.
- **Limit:** supported-platform and tool behavior are not target-host validation.

## S75-006 — iperf3 invocation reference

- **Publisher/revision:** ESnet, iperf3 3.21 documentation, accessed 2026-07-16.
- **URL:** https://software.es.net/iperf/invoking.html
- **Supports:** parallel, reverse, bidirectional, JSON, UDP controls.
- **Limit:** parallel streams may mask single-stream CPU limits.

## S75-007 — Linux interface statistics

- **Publisher/revision:** Linux kernel documentation 6.15, accessed 2026-07-16.
- **URL:** https://docs.kernel.org/6.15/networking/statistics.html
- **Supports:** standard, ethtool, netdev, and driver counter semantics.
- **Limit:** actual driver counter coverage must be inventoried.

## S75-008 — Linux network timestamping

- **Publisher/revision:** Linux kernel documentation 6.15, accessed 2026-07-16.
- **URL:** https://docs.kernel.org/6.15/networking/timestamping.html
- **Supports:** software/hardware timestamp API and PTP clock dependencies.
- **Limit:** no claim that the HaloFPX USB4 interface implements timestamps.

## S75-009 — HIP coherence control

- **Publisher/revision:** AMD ROCm HIP 6.4.1 documentation, accessed 2026-07-16.
- **URL:** https://rocm.docs.amd.com/projects/HIP/en/docs-6.4.1/how-to/hip_runtime_api/memory_management/coherence_control.html
- **Supports:** coarse/fine-grained visibility and synchronization semantics.
- **Limit:** target ROCm version and buffer path remain open.

## S75-010 — HIP asynchronous execution

- **Publisher/revision:** AMD ROCm HIP 6.4.3 documentation, accessed 2026-07-16.
- **URL:** https://rocm.docs.amd.com/projects/HIP/en/docs-6.4.3/how-to/hip_runtime_api/asynchronous.html
- **Supports:** asynchronous copies, streams, events, and synchronization.
- **Limit:** documentation does not prove transport integration correctness.
