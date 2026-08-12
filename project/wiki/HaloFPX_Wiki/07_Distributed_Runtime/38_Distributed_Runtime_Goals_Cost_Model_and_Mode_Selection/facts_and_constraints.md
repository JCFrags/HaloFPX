---
section_id: "38"
title: "Cost Model Facts and Constraints"
status: "needs-machine-validation"
last_verified: "2026-07-17"
applies_to:
  repositories: ["charlie12345/ROCmFPX@a5605a72768c6562241b248e268e33dc92787394", "ggml-org/llama.cpp@788e07dc91d266ad3162a1ce9037665656269689"]
  software_versions: []
  hardware_revisions: ["dual Strix Halo premise"]
related_sections: ["40", "41", "42", "43", "44", "47", "48", "51"]
---

# Cost model facts and constraints

## Source-backed constraints

- **[VERIFIED]** At commit `788e07d`, `llama.cpp` calls its RPC backend proof-of-concept, fragile, and insecure; it offloads computations, distributes weights and KV across devices by available memory by default, and exposes `--tensor-split` [S38-02]. This is not evidence of Megatron-style two-rank tensor parallelism.
- **[VERIFIED]** At commit `9354f22`, vLLM documents independent data-parallel engines with independent KV caches and says routing can consider running requests, waiting requests, and KV state [S38-03].
- **[VERIFIED]** Orca motivates iteration-level scheduling because generative requests have different iteration counts and supports selective batching [S38-04].
- **[VERIFIED]** Megatron's tensor-parallel scheme inserts communication around row/column-sharded transformer matrices; its published results concern datacenter GPUs, not Strix Halo over USB4 [S38-05].
- **[VERIFIED]** Exact speculative sampling can preserve the target distribution when the published acceptance/correction rule is followed [S38-06]. A heuristic accept rule does not inherit that guarantee.

## Quantitative notation

Measure every term by phase (`prefill`, `decode`) and batch/concurrency bucket. All tail comparisons use p99, not link-rate arithmetic.

| Symbol | Meaning | Unit |
|---|---|---|
| `C_r(m,b,s)` | rank `r` compute time for mode `m`, batch `b`, sequence state `s` | ms/iteration |
| `X_m(B)` | end-to-end transfer time for payload `B`, including copies/protocol | ms |
| `A_m(B,op)` | collective completion time for payload and operation | ms |
| `Q_m(lambda)` | admission plus scheduler queue delay at offered load `lambda` | ms |
| `K_m` | cache lookup/load/recompute cost; misses include recomputation | ms |
| `J_m` | residual scheduling, transport, and OS jitter | ms |
| `H_m` | fixed per-request/session setup and handshake | ms |
| `F_m` | expected failure/retry penalty over the observation window | ms |

**[RECOMMENDATION]** For request `i`, estimate:

`T_i(m) = H_m + Q_m + T_prefill,m + sum_t max_r(C_r,m,t) + sum_t(A_m,t + X_m,t) + K_m + J_m + F_m`.

Report TTFT, inter-token latency (ITL), end-to-end latency, accepted tokens/s, requests/s, usable context/model capacity, peak memory per rank, energy/request, cache-hit ratio, and error/fallback rate. Report p50/p95/p99 and confidence intervals; do not sum independently measured p99 components as if that were a statistically valid end-to-end p99.

## Mode cost signatures

| Mode | Capacity/compute benefit | Dominant extra cost | Failure domain |
|---|---|---|---|
| Full replication | two independent copies; aggregate throughput | duplicate model memory; routing/queue imbalance | one node can continue |
| Remote draft | target plus smaller drafter; possible lower ITL | draft rounds, verification, rejection/rollback, two KV states | target can fall back to ordinary decode |
| Two-way TP | roughly half shardable weights per rank; parallel GEMMs | collectives at transformer boundaries every layer | either rank loss aborts the request |
| Pipeline | model capacity via contiguous layer split | activation transfers and bubbles, especially decode batch 1 | either stage loss aborts request |
| MoE hybrid | expert capacity and conditional compute | token dispatch/all-to-all, imbalance, replicated hot experts | mapping-dependent |

**[ASSUMPTION]** The two hosts are matched and have two independently usable USB4 paths. Link aggregation, ordering, GPU-direct behavior, and stable p99 latency remain unverified here.

## Current operational comparison

- **[MEASURED]** `charlie12345/rocmfp4-llama@4860505e` ran with nimo-2 as the LAN-facing coordinator/model owner and nimo-1 as the private RPC worker [S38-L01].
- **[MEASURED]** The command explicitly selected `--device RPC0,ROCm0 --split-mode layer --tensor-split 1,1`; MPTCP reported two subflows over the two private USB4 subnets [S38-L01].
- **[INFERENCE]** This is contiguous remote/local device placement through ggml RPC, not evidence of Megatron-style tensor parallelism or a pipelined microbatch implementation.
- **[RECOMMENDATION]** Retain this configuration as the Phase-2 bring-up control. Measure it against single-node/reduced-model, single-rail, dual-rail, and later HaloFPX modes before changing the selector.

## Objective classes

**[RECOMMENDATION]** Product owners must choose numeric bounds for: interactive TTFT/ITL p99; saturated throughput at an admitted-concurrency cap; largest model/context that remains within memory safety margin; and degraded-mode availability. Until then, optimize no single metric in isolation.
