# Metric Definitions

> **Wiki status:** Proposed · **Evidence state:** D0 — design only · **Last reviewed:** 2026-07-17  
> **Machine-validation status:** Not run. Missing measurements are `INSUFFICIENT_EVIDENCE`, never an implicit pass.


Definitions are normative. A collector may add fields but may not silently change clock boundaries or denominators.

## LLM latency and throughput

| ID | Metric | Definition |
|---|---|---|
| `LAT-TTFT-C` | Client TTFT | Client send timestamp to first non-empty output-token event. Includes client/server network, queueing, prefill, and first decode. |
| `LAT-TTFT-S` | Server TTFT | Request accepted/queued by server to first token emitted by server. |
| `LAT-ITL` | Inter-token latency | Difference between consecutive non-empty token event timestamps after the first token. Report distribution across token events and per-request p95. |
| `LAT-TPOT` | Time per output token | `(last_token - first_token) / (output_tokens - 1)` for requests with at least two output tokens. |
| `LAT-E2E` | End-to-end latency | Client send to final content token; exclude terminal SSE `[DONE]` or empty frame. |
| `PREFILL-TPS` | Prefill throughput | `uncached_prompt_tokens / engine_prefill_seconds`; cached prefix tokens are excluded from numerator and denominator. |
| `DECODE-TPS` | Decode throughput | `(output_tokens - 1) / (last_token - first_token)` for a single request; aggregate output token rate is reported separately. |
| `GOODPUT-OTPS` | SLO-qualified output goodput | Output tokens/sec from requests satisfying all latency and correctness SLOs. |
| `REQ-THROUGHPUT` | Request throughput | Successfully completed requests per second. |

The vLLM metrics design uses the same TTFT and inter-token/TPOT vocabulary; `llama-bench` separately exposes prompt processing (`pp`), text generation (`tg`), and combined (`pg`) tests. [[SRC-008]](../references/Sources.md#src-008) [[SRC-015]](../references/Sources.md#src-015) [[SRC-016]](../references/Sources.md#src-016)

## Cache metrics

| ID | Metric | Formula |
|---|---|---|
| `CACHE-PREFIX-TOKEN-HIT` | Eligible prefix token hit rate | `sum(cached_prompt_tokens) / sum(eligible_prefix_tokens)` |
| `CACHE-REQUEST-HIT` | Request hit rate | `requests_with_cached_tokens / eligible_requests` |
| `CACHE-SAVED-PREFILL-MS` | Saved prefill time | Matched cold/no-cache prefill minus cache-hit prefill |
| `CACHE-EFFICIENCY` | Time saved per cached token | `CACHE-SAVED-PREFILL-MS / cached_prompt_tokens` |

`llama-server` can report cached and processed prompt progress; its own documentation also warns that prompt caching can change numerical execution because prompt and generation batch shapes differ. Correctness trials therefore test cache-on and cache-off separately. [[SRC-009]](../references/Sources.md#src-009)

## Disk and memory

| ID | Metric | Definition |
|---|---|---|
| `IO-READ-AMP-COLD` | Cold read amplification | Attributable physical block-read bytes during startup divided by unique logical model bytes loaded. |
| `IO-READ-AMP-WARM` | Warm read amplification | Same formula after verified OS page-cache warm state. |
| `IO-WRITE-AMP` | Inference write amplification | Attributable block-write bytes divided by output artifact bytes intentionally written. Report absolute bytes if denominator is zero. |
| `MEM-HEADROOM` | Available-memory headroom | Minimum `MemAvailable` divided by physical memory, plus absolute GiB. |
| `PSI-*` | Pressure stall | CPU, memory, and IO PSI `some`/`full` totals and rolling averages. |

Use cgroup-v2 `io.stat` where possible, with device-level counters as a cross-check. Linux documents `drop_caches` as a testing/debugging control that may itself cause significant I/O and CPU work; reboot is the preferred power-on-cold method. [[SRC-019]](../references/Sources.md#src-019) [[SRC-020]](../references/Sources.md#src-020) [[SRC-021]](../references/Sources.md#src-021)

## USB4

| ID | Metric | Definition |
|---|---|---|
| `USB4-NEGOTIATED` | Link state | Per-lane RX/TX speed × active lane count, recorded from sysfs on both hosts. |
| `USB4-REF-GOODPUT` | Reference application capacity | Median `iperf3` application goodput for direction/stream/MTU profile. |
| `USB4-REF-IFRATE` | Reference interface-byte rate | Interface-counter byte rate measured over the same `iperf3` reference interval. |
| `USB4-UTIL-APP` | Application-layer utilization | Runtime/RPC application payload bytes per second divided by direction-matched `USB4-REF-GOODPUT`. Use only when runtime payload counters exist. |
| `USB4-UTIL-IF` | Interface-layer utilization | Inference interface-counter bytes per second divided by direction-matched `USB4-REF-IFRATE`. |
| `USB4-RETX-RATE` | TCP retransmit rate | Retransmitted segments or bytes divided by sent segments or bytes over the interval. |
| `USB4-RENEGOTIATIONS` | Link instability | Count of lane/speed/domain reconnect changes during a normal run. |

Gate utilization against a measured same-layer reference, not the marketing signaling rate. Never divide interface bytes by application goodput or vice versa. Linux supports host-to-host USB4NET and exposes link speed/lane attributes; USB-IF describes USB4 signaling capability but actual host implementation and protocol overhead must be measured. [[SRC-010]](../references/Sources.md#src-010) [[SRC-011]](../references/Sources.md#src-011) [[SRC-012]](../references/Sources.md#src-012)

## Utilization, energy, and thermals

| ID | Metric | Definition |
|---|---|---|
| `CPU-UTIL` | CPU busy | Per-core and process CPU utilization with frequency and run queue context. |
| `GPU-UTIL` | GPU busy | Driver/SMU busy percentage; pair with memory-busy and clocks. |
| `ENERGY-GROSS` | Gross energy | Integral of both nodes' wall power over the run, when external meters exist. |
| `ENERGY-TOKEN` | Energy efficiency | Gross joules divided separately by prompt, output, and total tokens. |
| `THERMAL-MARGIN` | Thermal margin | Exposed critical/throttle threshold minus maximum measured temperature. |
| `THROTTLE-EVENTS` | Throttling | Count and duration of hardware/firmware throttle indicators. |

Driver telemetry is diagnostic and must not be assumed equivalent to wall power. AMDGPU `gpu_metrics` can expose temperature, frequency, utilization, power, throttler, fan, and APU CPU statistics in one snapshot. [[SRC-013]](../references/Sources.md#src-013)
