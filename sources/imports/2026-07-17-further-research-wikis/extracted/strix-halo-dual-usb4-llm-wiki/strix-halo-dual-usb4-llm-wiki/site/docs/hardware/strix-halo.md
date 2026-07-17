---
title: Strix Halo platform boundary
status: sourced facts and measurement gates
---

# Strix Halo platform boundary

This wiki models **two separate Ryzen AI Max+ 395 systems**. It does not model a cache-coherent, shared-memory dual-socket computer.

## Relevant sourced facts

| Property | Value | Evidence | Modeling consequence |
|---|---:|---|---|
| Former codename | Strix Halo | **SOURCED FACT** — [AMD product specification](https://www.amd.com/en/products/processors/laptop/ryzen/ai-300-series/amd-ryzen-ai-max-plus-395.html) | Names the target APU family. |
| Maximum system memory | 128 GB per system | **SOURCED FACT** — AMD product specification | Two systems provide two local memory domains, not one transparent 256 GB allocation. |
| Memory interface | 256-bit LPDDR5x; up to LPDDR5x-8000 listed | **SOURCED FACT** — AMD product specification | Local model execution can use a high-bandwidth unified-memory architecture, but usable model memory and achieved bandwidth remain runtime/OEM measurements. |
| Native USB4 ports | 2, each listed at 40 Gb/s | **SOURCED FACT** — AMD product specification | Makes a two-cable topology physically plausible. It does not establish dual-link bonding or effective payload throughput. |
| Integrated GPU configuration | 40 graphics cores listed | **SOURCED FACT** — AMD product specification | Confirms the intended accelerator class. No compute-time value is inferred from core count. |

The structured copy of these facts is in [`data/hardware_facts.csv`](../../data/hardware_facts.csv).

## Memory-domain model

For node/rank \(r\in\{0,1\}\), require:

\[
W_r + K_r + A_r + R_r + O_r \le M_{usable,r}-M_{safety,r}.
\]

Where:

- \(W_r\): loaded weights, quantization scales, padding, and duplicated tensors;
- \(K_r\): KV cache for sessions and layers owned by the rank;
- \(A_r\): activation, boundary, collective, and pipeline buffers;
- \(R_r\): runtime workspaces, graphs, kernels, and queues;
- \(O_r\): operating-system and co-resident service allocations;
- \(M_{usable,r}\): **MEASURED INPUT REQUIRED**, not the advertised physical maximum;
- \(M_{safety,r}\): project-defined safety reserve.

### What “256 GB across two systems” may mean

It can mean that the placement assigns some immutable weights and KV shards to node A and the rest to node B. It must not mean that either process can dereference the other node's memory, that a single allocation can span both machines, or that remote memory has local-memory semantics.

## Weight-capacity lower bound

For \(P\) parameters and ideal packed weight precision \(q\) bits:

\[
W_{ideal}=\frac{Pq}{8}.
\]

This is a **CALCULATED LOWER BOUND**, not a load-size estimate. Real checkpoints add some combination of:

- block scales, zero points, codebooks, and group metadata;
- tensors stored at higher precision;
- row/alignment padding;
- duplicated embeddings or output heads;
- deserialization/copy buffers;
- graph and kernel workspace;
- allocator fragmentation.

The memory gate must use measured resident allocations for the selected runtime and checkpoint.

## KV-capacity model

For a conventional decoder with \(L\) attention layers, \(H_{kv}\) KV heads, head dimension \(d\), and \(b_{kv}\) bytes per element:

\[
K_{token}=2L H_{kv} d b_{kv}.
\]

The factor two is for K and V. For context \(C\), active sequences \(Q\), and local layer set \(\mathcal L_r\):

\[
K_r(C,Q)=2CQ\left(\sum_{l\in\mathcal L_r}H_{kv,l}d_l\right)b_{kv}.
\]

Use actual paged-cache allocation granularity, prefix sharing, sliding-window rules, and KV quantization when present.

## Runtime boundary

AMD has published a distributed llama.cpp RPC example on Ryzen AI Max+ systems. That is evidence that an RPC implementation route exists on this platform; it is not evidence that the exact two-node USB4 topology, tensor collectives, expert routing, or every ownership layout in this wiki is already implemented. See [AMD's technical article](https://www.amd.com/en/developer/resources/technical-articles/2026/how-to-run-a-one-trillion-parameter-llm-locally-an-amd.html).

## Platform gates

Before a placement can be called feasible:

1. Record exact system/OEM, memory capacity, firmware, OS/kernel, graphics driver, runtime, and power profile.
2. Measure usable memory after the runtime initializes but before model load.
3. Load the exact checkpoint and record peak as well as steady resident memory.
4. Run at maximum planned context and concurrency without paging or uncontrolled eviction.
5. Verify both intended USB4 ports are independently exposed and stable.
6. Verify tensor dtype/layout support on both nodes; do not assume a network path can read accelerator memory directly.
7. Record sustained clocks, temperature, and power behavior during a long run.

## Non-claims

This page makes no claim about tokens/s, time to first token, inter-token latency, local memory bandwidth achieved by a runtime, USB4 payload bandwidth, direct accelerator-to-accelerator transfer, or dual-link aggregation. Those are measured inputs to the cost model.
