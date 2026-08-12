---
section_id: "76"
title: "Distributed Benchmark Procedures and Checks"
status: "needs-machine-validation"
last_verified: "2026-07-16"
applies_to:
  repositories:
    - "ggml-org/llama.cpp@788e07dc91d266ad3162a1ce9037665656269689"
    - "charlie12345/ROCmFPX@a5605a72768c6562241b248e268e33dc92787394"
  software_versions: []
  hardware_revisions: ["planned matched dual Strix Halo"]
related_sections: ["73", "74", "75", "78", "80"]
---

# Procedures and Checks

These procedures define future experiments. They do not create measurements merely by being written.

## 1. Gate prerequisites

Require frozen source/build/model hashes; exact node manifests; Section 74 single-node results on both nodes; Section 75 fabric profiles; deterministic request traces; synchronized clocks or a single monotonic timing authority; and a correctness oracle from Section 78.

## 2. Establish controls

Run, in randomized blocks:

1. rank 0 alone;
2. rank 1 alone;
3. two independent replicas with identical router load;
4. each distributed candidate.

Do not compare a warm distributed run with a cold single-node run. Match model residency, cache state, prompt order, request arrival process, output budget, sampling seed/profile, power mode, and warmup.

## 3. Instrument a common event schema

Each request records timestamps for admission, queue exit, prompt start/end, every emitted token, draft start/end, target verification, each send/receive/collective, cancellation, completion, and fallback. Per rank record compute/idle intervals, bytes, messages, memory, power, clocks, thermals, errors, and queue depth. Use stable request, session, rank, plan, model, build, and run IDs.

## 4. Mode-specific sweeps

### Replication

Sweep concurrency and arrival rate. Report per-replica balance, queueing, affinity/cache effects, aggregate goodput, tails, and node-loss behavior.

### Native MTP and local/remote draft

Sweep draft length and configured acceptance controls. Record proposed, accepted, rejected, and target-verified tokens; draft and verification time; transfer bytes; cancellation; and emitted-token correctness. Compare local draft, remote draft on each link, and native MTP against the same target-only run.

### Tensor parallel

Sweep tensor split and relevant batch sizes. Record each collective by type/bytes/duration, per-rank kernels, imbalance, synchronization, and communication overlap.

### Pipeline

Sweep layer boundary, microbatch count, and concurrency. Record stage service time, activation bytes, queue wait, fill/drain, measured idle fraction, memory, and end-to-end tails.

### MoE hybrid

Use repeatable routing traces. Record expert selection counts, remote/local expert tokens, bytes, hot/cold expert residency, imbalance, misses, and quality. Never infer expert placement from aggregate throughput alone.

## 5. Break-even analysis

Fit no universal curve until raw paired cells pass review. For each predeclared workload bucket, bootstrap or otherwise compute the Section 73-approved paired interval for throughput and latency ratios. Retain cell counts and missing/failure outcomes. Cross-validation or held-out traces are required before planner publication.

## 6. Degradation tests

After safe non-destructive timeout tests, route cable pulls, process kills, OOM, corruption, and disk-full cases to Section 80. Verify no invalid rank-local cache state is accepted; recovery must miss/recompute or use a validated fallback.

## 7. Promotion checklist

- exact commands/configuration and raw schema preserved;
- source/build/model/topology hashes resolve;
- single-node and replication controls match;
- correctness/quality passes;
- p50/p95/p99 and uncertainty reported;
- compute versus communication attribution present;
- failure outcomes included, not discarded;
- model-specific validity range and fallback documented.
