# Benchmark Methodology

> **Wiki status:** Proposed · **Evidence state:** D0 — design only · **Last reviewed:** 2026-07-17  
> **Machine-validation status:** Not run. Missing measurements are `INSUFFICIENT_EVIDENCE`, never an implicit pass.


## Experimental unit

A **run** is one pinned SUT configuration, one workload cell, one cache state, and one uninterrupted measurement interval. A **run block** is a randomized or counterbalanced collection of comparable runs executed under one environment freeze. A **release dataset** contains at least one independent reproduction block.

## Measurement layers

| Layer | Purpose | Examples |
|---|---|---|
| Client-observed | User-visible behavior including queue and network | TTFT, token event times, E2E latency, success |
| Server/engine | Separate queue, prefill, decode, and cache work | prompt eval time, decode time, cached tokens |
| Host | Diagnose CPU, memory, disk, scheduler, and pressure | `pidstat`, cgroup `io.stat`, PSI, faults |
| Accelerator | Utilization, clocks, memory, power, thermal, throttling | AMD SMI, `gpu_metrics`, profiler |
| Link | Negotiated state, payload throughput, errors, retransmits | USB4 sysfs, interface counters, `iperf3`, `ss` |
| External | Ground-truth facility variables where available | wall power, ambient temperature |

## Repetition and duration defaults

| Trial class | Warm-up | Measured sample minimum | Duration rule |
|---|---:|---:|---|
| `llama-bench` prefill/decode microbenchmark | 3 iterations | 10 iterations/cell | Continue until 95% CI width ≤5% or 30 iterations |
| Cold start | none after verified cold state | 7 boots/cell | One load and canary per boot |
| Warm start/cache trial | 2 primes | 20 requests/cell | Randomize cache-state order where possible |
| Streaming latency | 20 requests | 200 successful requests/cell | At least 10 minutes |
| Concurrency/load point | 2 minutes | 1,000 requests or 15 minutes | Whichever is longer |
| Long context | 1 prime | 5 requests/length | Stop only at declared limit or safe failure |
| Fault type/severity | 1 dry rehearsal | 3 injections | Include pre-, during-, and post-fault windows |
| Stable soak | 30 minutes | 72 hours | Mixed workload; no evidence gaps >60 s |

## Run order and controls

- Compare Node A, Node B, and dual-node using paired prompt IDs and identical generated-token limits.
- Use randomized AB/BA or Latin-square order inside run blocks; cold-start blocks are counterbalanced by day/boot order.
- Hold model, quantization, tokenizer, prompt tokens, context, sampling, CPU/GPU settings, ambient target, and client location fixed.
- Disable unrelated jobs, auto-updates, backups, indexers, display workloads, and power-saving transitions not part of the declared profile.
- Report all samples. Exclusion is allowed only under a predeclared invalidation rule and must retain the record.

## Statistics

Report count, median, mean, standard deviation, MAD, P90, P95, P99, min, max, and bootstrap 95% confidence intervals. Release comparisons use paired ratios when prompt IDs align. A threshold crossing is a hard failure only when the configured confidence rule is met; otherwise classify as `WARN_RETEST`, never silently average it away.

## Saturation search

For serving throughput, increase offered load in geometric steps until one of these occurs:

- p95 or p99 latency exceeds the declared SLO;
- queue depth grows for three consecutive windows;
- goodput falls while offered load rises;
- error/cancel rate breaches the gate;
- CPU, memory, GPU, disk, or link pressure indicates an unsafe limit.

The production operating point is the highest load with at least 20% request-rate headroom and all SLOs met, unless a different headroom is explicitly approved.
