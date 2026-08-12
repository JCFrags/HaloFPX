# Results

Claim scope: all numbers below are `[MEASURED]` on `nimo-1` and `nimo-2` on 2026-07-17 with the exact environment in `README.md`. They are not USB4 link-layer limits and are not tensor-parallel measurements.

## Summary

| Mode | Direction | Rail A | Rail B | Aggregate or logical connection | Retransmits |
|---|---|---:|---:|---:|---:|
| Isolated TCP | nimo-1 → nimo-2 | 9.704 Gb/s | 9.813 Gb/s | n/a | 0 on each |
| Isolated TCP | nimo-2 → nimo-1 | 9.421 Gb/s | 9.671 Gb/s | n/a | 0 on each |
| Concurrent independent TCP | nimo-1 → nimo-2 | 10.519 Gb/s | 10.520 Gb/s | 21.040 Gb/s | 0 on each |
| Concurrent independent TCP | nimo-2 → nimo-1 | 10.270 Gb/s | 10.270 Gb/s | 20.540 Gb/s | 0 on each |
| MPTCP, two subflows | nimo-1 → nimo-2 | n/a | n/a | 20.714 Gb/s | 1 |
| MPTCP, two subflows | nimo-2 → nimo-1 | n/a | n/a | 20.687 Gb/s | 16 |

`ss -M` recorded `subflows_total:2` on both the MPTCP data socket and control socket during each direction. Thus `[MEASURED]` the currently deployed MPTCP configuration aggregated both private USB4/Thunderbolt-net paths for this workload.

## Latency

| Condition | Rail A RTT min/avg/max/mdev | Rail B RTT min/avg/max/mdev | Loss |
|---|---|---|---:|
| Idle, 200 packets | 0.107/0.129/0.193/0.008 ms | 0.129/0.135/0.214/0.005 ms | 0% |
| Concurrent forward load, 600 packets | 0.181/0.576/0.721/0.050 ms | 0.128/0.368/0.515/0.074 ms | 0% |
| Concurrent reverse load, 600 packets | 0.160/0.317/0.357/0.017 ms | 0.127/0.317/0.392/0.018 ms | 0% |

## Interpretation

- `[MEASURED]` Two independent connections and one MPTCP connection both reached about 20.5–21.0 Gb/s aggregate. This is the correct stable transport baseline for later comparisons.
- `[MEASURED]` A one-rail receive path reported approximately 100% iperf process CPU and about 9.4–9.8 Gb/s. This suggests a per-flow or receive-processing bottleneck, but iperf's process statistic is not a system-wide CPU profile.
- `[INFERENCE]` USB4STREAM can only be called better for this project if a matched test improves application-relevant latency, CPU cost, or tensor throughput beyond this baseline without losing stability, recovery, and fallback behavior.
- `[OPEN]` The current run does not isolate scheduler, IRQ, checksum/offload, queue-count, socket-buffer, or copy-path contributions.
- `[OPEN]` The current run does not measure llama.cpp RPC messages, tensor sizes, token latency, throughput, quality, or bidirectional tensor exchange.

## Safety observations

- Both rails remained `UP,LOWER_UP` with zero RX/TX errors reported after the run.
- No new kernel warnings at priority warning-or-higher were recorded from 17:58 through 18:08 PDT.
- No experiment `iperf3` process or port remained after the one-shot tests.
- No systemd unit entered failed state.
- Cumulative interface TX dropped counters were 8–9 and predate or span more than this run; no before-counter was captured, so they are not attributed to the experiment.

## Next matched experiments

1. Capture `mpstat`, softirq, IRQ affinity, queue, and perf profiles during the same matrix.
2. Add repeated trials and confidence intervals instead of one sample per cell.
3. Replay representative llama.cpp RPC tensor payload sizes and directionality.
4. Compare MPTCP against any USB4STREAM candidate with identical payloads, CPU policy, kernel, duration, and telemetry.
5. Add controlled link-loss and fallback only after a separate failure-injection card and rollback review.

