---
title: Measurement worksheet
status: blank operational template
---

# Measurement worksheet

No performance values are prefilled. Copy this page or use [`data/measurement_template.csv`](../../data/measurement_template.csv) for each campaign.

## Campaign identity

| Field | Value |
|---|---|
| Campaign ID | |
| Date/time UTC | |
| Operator | |
| Objective | capacity / TTFT / ITL / throughput / concurrency / resilience |
| Node A system/firmware | |
| Node B system/firmware | |
| OS/kernel and drivers | |
| Runtime revision/build | |
| Model revision/quantization/hash | |
| Placement file/revision | |
| Cable and physical-port map | |
| Power/thermal policy | |

## Memory

| Rank | Usable before load | Loaded weights | Quant metadata/padding | KV at test load | Workspace/buffers | Peak resident | Safety margin | Pass? |
|---:|---:|---:|---:|---:|---:|---:|---:|---|
| 0 | | | | | | | | |
| 1 | | | | | | | | |

Required inequality:

\[
W_r+K_r+A_r+R_r+O_r\le M_{usable,r}-M_{safety,r}.
\]

## Per-link calibration

| Path | Direction | Size region | Samples | \(\ell\) | \(B\) | p95 | p99 | Errors/retries | Fit residual note |
|---|---|---|---:|---:|---:|---:|---:|---:|---|
| 1 | A→B | small | | | | | | | |
| 1 | A→B | bulk | | | | | | | |
| 1 | B→A | small | | | | | | | |
| 1 | B→A | bulk | | | | | | | |
| 2 | A→B | small | | | | | | | |
| 2 | A→B | bulk | | | | | | | |
| 2 | B→A | small | | | | | | | |
| 2 | B→A | bulk | | | | | | | |

## Concurrent-link gate

| Test | Message/chunk size | \(B_1\) | \(B_2\) | \(B_{both}\) | \(\eta_{link}\) | p99 change | Reorder peak | Pass? |
|---|---:|---:|---:|---:|---:|---:|---:|---|
| independent flows | | | | | | | | |
| application stripe | | | | | | | | |
| full duplex | | | | | | | | |
| control under bulk | | | | | | | | |

Aggregation is disabled until this gate passes.

## Local compute baseline

| Workload | Prompt \(N\) | Decode batch \(Q\) | Context | Prefill \(C_1\) | ITL \(T_{t,1}\) | p95 | p99 | Peak memory |
|---|---:|---:|---:|---:|---:|---:|---:|---:|
| | | | | | | | | |

## Tensor-parallel record

| Quantity | Value | Evidence |
|---|---:|---|
| Collectives observed per forward | | trace |
| \(m_{AR}\) | | trace |
| Message size \(S\) | | trace/formula |
| TP compute efficiency \(\eta_{TP}\) | | measured decomposition |
| Collective communication time | | measured |
| Required \(B\) from break-even | | calculated |
| Actual matched-size \(B\) | | measured |
| Correctness pass | | test |
| Disposition | | GO / CONDITIONAL / NO-GO |

## Layer split / pipeline record

| Cut \(k\) | \(c_A\) | transfer \(x\) | \(c_B\) | Rank 0 peak | Rank 1 peak | \(M\) | Predicted makespan | Measured makespan | p99 | Pass? |
|---:|---:|---:|---:|---:|---:|---:|---:|---:|---:|---|
| | | | | | | | | | | |

## MoE expert record

| Layer group | Tokens | Top-k | \(\rho_l\) mean | \(\rho_l\) p99 | Remote assignments | Bytes | Dispatch+return | Queue tail | Saved local work | Pass? |
|---|---:|---:|---:|---:|---:|---:|---:|---:|---:|---|
| | | | | | | | | | | |

## Remote speculation record

| \(\gamma\) | Draft time | Verify time | Round bytes | RTT cost | Accepted-length distribution | \(E[K]\) | Baseline budget | Measured round time | Exactness pass | Break-even? |
|---:|---:|---:|---:|---:|---|---:|---:|---:|---|---|
| | | | | | | | | | | |

## Final decision record

```text
Primary objective:
Hard gates failed/passed:
Formula and substituted measured values:
Nominal-floor triage (not performance):
Correctness result:
Sustained p50/p95/p99:
Capacity and safety margin:
Failure/recovery behavior:
Disposition: GO / CONDITIONAL / NO-GO
Scope of decision:
Expiration triggers:
Reviewer:
```
