---
title: Memory and KV cost model
status: symbolic model
---

# Memory and KV cost model

## Weight lower bound

For \(P\) parameters stored at an ideal \(q\)-bit packing:

\[
W_{ideal}=P\frac{q}{8}.
\]

This is a **CALCULATED LOWER BOUND**, not actual checkpoint or runtime residency. Real deployments add some combination of quantization scales/zeros, group metadata, tensor padding, duplicated embeddings, file/container metadata, dequantization buffers, graph allocations, kernels/workspaces, allocator fragmentation, runtime state, and OS memory.

For two-node placements, total physical memory is not sufficient. Each rank must pass independently:

\[
W_r+K_r+A_r+R_r+M_{OS,r}\le M_{physical,r}.
\]

Prefer measuring \(M_{usable,r}\) after boot and runtime initialization, then require:

\[
W_r+K_r+A_r+R_r\le M_{usable,r}-M_{safety}.
\]

## KV bytes

For a transformer with \(L\) layers, \(H_{kv}\) KV heads, explicit head dimension \(d\), and \(b_{kv}\) bytes per element, whole-model K+V storage for one token of one sequence is:

\[
K_{token}=2LH_{kv}db_{kv}.
\]

For context \(c_i\) on sequence \(i\):

\[
K_i=c_iK_{token}.
\]

For a set of sessions \(\mathcal S\), ignoring block allocator slack and metadata:

\[
K_{total}=K_{token}\sum_{i\in\mathcal S} c_i.
\]

### Layer-local KV

A contiguous layer owner with layer set \(\mathcal L_r\) stores:

\[
K_{r,token}=2b_{kv}\sum_{l\in\mathcal L_r}H_{kv,l}d_l.
\]

For uniform layers and \(L_r\) owned layers:

\[
K_{r,token}=2L_rH_{kv}db_{kv}.
\]

This is why a contiguous split does not require steady-state KV traffic: the rank that computes attention for a layer owns that layer's KV.

### Tensor-parallel KV

When KV heads divide across two ranks:

\[
K_{r,token}=\frac{K_{token}}{2}.
\]

This requires \(H_{kv}\) or the implementation's KV-head grouping to admit an exact two-way split. Multi-query attention with one KV head cannot be evenly head-sharded; replication or a different partition is required and must be included in memory and communication accounting.

### Replicated decode KV

The model is duplicated, but each session's KV exists only on its assigned replica:

\[
K_A=K_{token}\sum_{i\in\mathcal S_A}c_i,
\qquad
K_B=K_{token}\sum_{i\in\mathcal S_B}c_i.
\]

Session affinity is therefore a correctness requirement, not only a scheduler preference.

## KV migration

Moving an existing session with context \(c\) requires at least:

\[
V_{KV\_move}=cK_{token}.
\]

Actual transfer can be larger because of block layout, padding, quantization metadata, rope/position state, and runtime serialization. Migration is justified only when measured future benefit exceeds transfer plus import cost:

\[
T_{move}=\ell+\frac{V_{KV\_move}}{B}+T_{export}+T_{import},
\]

\[
T_{move}<G\,\Delta T_{decode},
\]

where \(G\) is remaining generated tokens and \(\Delta T_{decode}\) is measured per-token savings after migration.

## Activation and transient memory

Communication volume does not equal peak activation memory. Prefill kernels may allocate attention workspaces, dequantization tiles, graph buffers, and boundary staging. Pipeline schedules retain multiple microbatch tensors. Expert service can queue routed vectors on both nodes.

For a layer boundary with \(M\) in-flight buffers of \(V_{mb}\) bytes:

\[
A_{boundary}\ge M V_{mb}
\]

before transport/runtime overhead. Pipeline GO therefore requires both a service-time gate and a queue-memory gate.

## Memory-driven cut selection

Do not select \(k=L/2\) solely by layer count. Per-layer weight and compute can differ because of:

- embeddings and LM head;
- MoE expert weights;
- nonuniform layer types;
- quantization padding;
- context-dependent KV and workspace;
- backend-specific fusion and graph allocation.

Measure or inspect per-tensor residency and choose \(k\) to satisfy both node budgets with a safety margin. For throughput pipelines, balance measured service time after satisfying memory.
