---
title: Benchmark and calibration plan
status: required measurement protocol
---

# Benchmark and calibration plan

This plan converts the repository's blank variables into measured inputs without back-solving from desired performance.

## 1. Freeze the test identity

Record before every campaign:

```text
campaign_id and UTC timestamps
system/OEM and memory capacity for node A/B
BIOS/firmware and power profile
OS, kernel/build, USB4/Thunderbolt driver
GPU driver and accelerator runtime
LLM runtime commit/build flags
model repository, revision, quantization, file hashes
placement YAML and schema version
cable make/length/certification and physical port mapping
ambient/thermal conditions
```

Change one controlled variable at a time. A result is not portable across materially different identities without revalidation.

## 2. Establish local baselines

For each model, context, active-sequence batch \(Q\), prompt length \(N\), quantization, and sampler:

- measure load time and peak/steady memory;
- measure prefill compute time and time to first token;
- measure decode inter-token latency and aggregate throughput;
- record p50, p95, p99, maximum, warm/cold distinction, and sample count;
- record stage/layer/operator timing when the runtime exposes it;
- run long enough to expose thermal/power steady state.

These are the baselines \(C_1\), \(T_{t,1}\), \(c_A\), and \(c_B\). Do not substitute published benchmark numbers from another machine.

## 3. Inventory the USB4 topology

Use the read-only helpers:

```bash
./tools/probe_linux_usb4.sh > measurements/topology-node-a.txt
```

or:

```powershell
.\tools\probe_windows_usb4.ps1 | Out-File measurements\topology-node-a.txt
```

Run on both nodes. Map OS interface names to physical ports by disconnecting/reconnecting one path at a time. Record controller/router ancestry where exposed.

## 4. Measure one path at a time

For each direction and path, sweep payload sizes that bracket actual mode messages. A suitable generic sweep is:

```text
4 B, 32 B, 256 B, 1 KiB, 4 KiB, 8 KiB, 32 KiB,
128 KiB, 512 KiB, 2 MiB, 8 MiB, 32 MiB, 128 MiB, 512 MiB
```

Add exact sizes from `data/worked_examples.csv` and the selected model/workload.

Measure two primitives:

1. **Ping-pong RTT** with a warm persistent connection and no remote compute. Fit one-way fixed cost from RTT/2 only as a symmetric-model approximation, and label it accordingly.
2. **One-direction streaming** with a sufficient window and repeated buffers. Measure application payload bytes, not interface counters alone.

Fit piecewise:

\[
t(V)=\ell+\frac{V}{B}.
\]

Use [`tools/fit_link_model.py`](../../tools/fit_link_model.py) rather than taking the maximum single observation. Inspect residuals; split small/bulk regimes when one affine fit is poor.

## 5. Measure directionality and duplex

Repeat A→B, B→A, and simultaneous A↔B. Record:

- payload bandwidth by direction;
- RTT/one-way proxy distributions;
- p95/p99 and maxima;
- CPU load, copies if traceable, and queue depth;
- retries/errors and interface resets.

Use direction-specific variables if asymmetry exceeds the project threshold.

## 6. Test two links concurrently

Run:

- path 1 alone;
- path 2 alone;
- two independent flows simultaneously;
- application striping at several chunk sizes;
- full-duplex simultaneous traffic;
- control traffic while bulk traffic is saturated.

Calculate:

\[
\eta_{link}=B_{both}/(B_1+B_2).
\]

Predeclare the pass threshold. Also require acceptable tails and bounded reorder buffers. Only a passing application-level test authorizes `validated_striping` in a placement file.

## 7. Measure runtime primitives

Use the exact runtime data path and actual tensor dtype/layout:

| Primitive | Sizes | Required outputs |
|---|---|---|
| point-to-point activation | per-token, decode batch, full prefill boundary | serialization, transfer, receive-ready time |
| p=2 all-reduce | TP message sizes | phase count \(m_{AR}\), sent/received bytes, barriers, total time |
| expert dispatch/return | routed-token distributions | bytes, assignments, phases, queue time |
| speculative RPC | ID-only and chosen probability protocol | request/response bytes and RTT |
| KV export/import | selected contexts | exported bytes, conversion, transfer, import, commit |

A TCP/iperf result is not a replacement for these measurements.

## 8. Measure mode-specific compute and behavior

### Tensor parallelism

- Trace exactly \(2L\) intended forward collectives or document the actual graph.
- Measure \(C_{TP}\) with the same workload and include collective staging.
- Derive \(\eta_{TP}=C_1/(2C_{TP,compute})\) only from a defensible decomposition.
- Evaluate the strict break-even formula; reject when its denominator is non-positive.

### Contiguous split

- Sweep cut layer \(k\).
- Record weight/KV/workspace by rank, \(c_A(k)\), \(c_B(k)\), boundary transfer, and token feedback.
- Verify each rank stores KV only for local layers.
- Compare end-to-end against the local baseline for the stated objective; capacity success is separate from speed.

### Pipeline

- Run \(M=1,2,4,8,\ldots\) independent microbatches/sessions up to the memory/SLO limit.
- Record fill, service interval, queue depth, p95/p99, and stage idle time.
- Compare measured \(M\) with \(M_{min}\) from the cost model.

### MoE expert placement

- Trace router decisions by layer and workload class.
- Compute \(\rho_l\), distribution of remote assignments, expert imbalance, and tail queue time.
- Measure dispatch, remote expert compute, return/combine, and local alternative.
- Never use the illustrative \(\rho=0.5\) row as evidence.

### Remote speculation

- Measure drafter block time \(T_d(\gamma)\), target verify time \(T_v(\gamma)\), accepted-length histogram, rollback cost, and bytes for each \(\gamma\).
- Greedy mode must match target greedy output token-for-token.
- Exact stochastic mode must pass seeded replay and distributional tests for the implemented protocol.
- Evaluate expected time from the empirical accepted-length distribution, not only a scalar acceptance mean.

### Replicated decode

- Assign sessions without cross-node model-path traffic.
- Measure aggregate throughput and per-session tails at increasing concurrency.
- Include router/queue policy and context-size imbalance.

### KV migration

- Measure export, format conversion, network, import, validation, and ownership commit separately.
- Verify source freeze/version semantics and failure rollback.
- Amortize against measured future per-token savings.

## 9. Correctness campaign

At minimum:

- fixed prompts and exact token IDs;
- greedy baseline equivalence;
- logits/hidden-state tolerances at every split boundary;
- maximum-context and ragged-batch cases;
- stop/grammar/tool-call behavior;
- cancellation at each protocol phase;
- stale, duplicate, reordered, and corrupted messages;
- process or cable failure during KV append, sampling, and migration;
- seeded stochastic and speculative tests.

Performance results are invalid until correctness passes.

## 10. Sustained system test

Run the planned concurrency and context mix through thermal steady state. Record p50/p95/p99, maximum queue memory, error/retry counts, temperatures, clocks, power profile, and link resets. A brief peak result cannot pass the operational gate.

## Measurement outputs

Populate [`data/measurement_template.csv`](../../data/measurement_template.csv) and preserve raw logs under a new `measurements/<campaign_id>/` directory. Keep calculated substitutions in a separate decision record so raw observations remain immutable.
