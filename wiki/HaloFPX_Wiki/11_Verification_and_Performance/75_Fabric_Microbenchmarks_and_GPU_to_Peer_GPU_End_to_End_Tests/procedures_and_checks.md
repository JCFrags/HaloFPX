---
section_id: "75"
title: "Fabric Benchmark Procedures and Checks"
status: "needs-machine-validation"
last_verified: "2026-07-16"
applies_to:
  repositories: []
  software_versions: ["iperf3 3.21", "HIP 6.4.x"]
  hardware_revisions: ["planned dual Strix Halo nodes; exact revisions open"]
related_sections: ["20", "49", "50", "54", "55", "73", "76"]
---

# Procedures and Checks

All procedures are plans. They produce **[MEASURED]** claims only after raw output, environment metadata, and the exact commands are preserved under the experiment authority.

## 1. Freeze environment and topology

Prerequisites: shell access to both nodes; root is not required for inventory but may be required for counters, affinity, or tuning. Do not change tuning in the inventory phase.

On both nodes capture:

```bash
uname -a
cat /etc/os-release
ip -details -statistics -json link show
ip -json address show
ip route show table all
ethtool -i IFACE
ethtool -k IFACE
ethtool -c IFACE
ethtool -g IFACE
ethtool -S IFACE
cat /proc/interrupts
cat /proc/softirqs
```

Record kernel/config hash, firmware, BIOS, ROCm/HIP, GPU identifiers, USB4 controller path, cable/port labels, MTU, qdisc, congestion control, CPU governor, power mode, temperature, and current affinity. Replace `IFACE` explicitly; never rely on a wildcard.

## 2. Prove path binding

For A only, B only, and A+B configurations, record addresses/routes and transport endpoint binding. Verify traffic counters advance only on intended interfaces. **[OPEN]** If a transport cannot prove link selection, its dual-link result is not admissible.

## 3. Clock and timer calibration

Test monotonic timer resolution/overhead on each host. Query hardware timestamp capability with `ethtool -T IFACE`. If reporting one-way delay, synchronize clocks, record method and offset/uncertainty before and after each run, and reject runs whose uncertainty is material relative to the reported percentile. Otherwise report RTT only. [S75-001][S75-008]

## 4. Host baseline

Pin tool version and preserve JSON:

```bash
iperf3 --version
iperf3 --server --bind SERVER_IP --port 5201 --one-off
iperf3 --client SERVER_IP --bind CLIENT_IP --time 30 --omit 5 --json
iperf3 --client SERVER_IP --bind CLIENT_IP --time 30 --omit 5 --reverse --json
iperf3 --client SERVER_IP --bind CLIENT_IP --time 30 --omit 5 --bidir --json
iperf3 --client SERVER_IP --bind CLIENT_IP --time 30 --omit 5 --parallel 4 --json
```

Run each direction separately before `--bidir`. Repeat with recorded socket windows, stream counts, and UDP target rates; retain loss/reordering/jitter. Do not use `--zerocopy` as the only result because it changes the host copy path. [S75-004][S75-005][S75-006]

## 5. Message latency and queue-depth sweep

Use a sequence-numbered request/reply harness with monotonic timestamps and payload validation. Randomize or block the run order according to the Section 73 plan. For each payload and queue depth, retain every observation, warmup marker, retry, timeout, duplicate, reorder, and corruption result. Report p50/p95/p99 plus confidence intervals and sample count.

## 6. Counter deltas and CPU cost

Capture before/after snapshots of interface stats, interrupts, softirqs, `nstat`, process CPU time, cycles, instructions, context switches, migrations, page faults, and power/temperature telemetry. Use `perf stat` only when permissions and event support are recorded; unsupported events remain null, not zero.

## 7. GPU-produce to peer-GPU-consume

Implement the boundary from [design implications](design_implications.md#decisive-gpu-test-boundary). Requirements:

- deterministic generation-tagged input and consumer-side validation;
- explicit buffer allocation type and device/host ownership;
- explicit HIP stream/event ordering and visibility operations;
- timestamps at producer, staging, transport, receive, consumer, and final completion;
- abort on stale generation, checksum mismatch, timeout, or incompatible buffer mapping;
- machine-readable one-record-per-operation output.

Run each transport in staged-copy and any claimed direct/zero-copy modes. A label such as “zero copy” is rejected unless the recorded buffer and DMA path substantiate it.

## 8. Contention and failure matrix

Repeat selected knee-point cells during decode, NVMe reads, NVMe writes, and mixed load. Then disable one link between trials and verify explicit degraded behavior. Live cable pulls and fault injection belong to Section 80 and require a separate safety card.

## 9. Evidence and promotion check

Before comparison, require exact command, source/tool version, run ID, topology ID, environment manifest, raw samples, counter snapshots, clock uncertainty, warmup policy, repetition count, and failure accounting. Hash raw artifacts. A summary without raw samples cannot establish p99 or corruption-free behavior.
