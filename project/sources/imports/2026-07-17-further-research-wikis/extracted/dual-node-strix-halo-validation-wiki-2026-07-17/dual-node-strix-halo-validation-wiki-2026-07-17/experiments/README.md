# Experiment Card Index

> **Wiki status:** Proposed · **Evidence state:** D0 — design only · **Last reviewed:** 2026-07-17  
> **Machine-validation status:** Not run. Missing measurements are `INSUFFICIENT_EVIDENCE`, never an implicit pass.



These cards are executable specifications. They define what must be measured, how controls are matched, what invalidates a run, and what a result can legitimately support. Completing a card document is `D0`; only schema-valid machine evidence can advance it to M1/M2/R1.

| ID | Experiment | Release profiles |
|---|---|---|
| [EXP-001](./EXP-001-Lab-Preflight-and-SUT-Freeze.md) | Lab Preflight and SUT Freeze | All |
| [EXP-002](./EXP-002-USB4-Link-Characterization.md) | USB4 Link Characterization | All |
| [EXP-003](./EXP-003-Power-On-Cold-Startup.md) | Power-On-Cold Startup and Model Load | All |
| [EXP-004](./EXP-004-Warm-and-Prompt-Cache-States.md) | Warm and Prompt-Cache State Trials | All |
| [EXP-005](./EXP-005-Prefill-Microbenchmark.md) | Prefill Microbenchmark | Scale-out and capacity extension |
| [EXP-006](./EXP-006-Decode-Microbenchmark.md) | Decode Microbenchmark | Scale-out and capacity extension |
| [EXP-007](./EXP-007-Streaming-TTFT-and-ITL.md) | Streaming TTFT, ITL, and Latency Decomposition | All |
| [EXP-008](./EXP-008-Concurrency-and-Throughput.md) | Concurrency and Throughput Matrix | All |
| [EXP-009](./EXP-009-Open-Loop-Saturation-and-Goodput.md) | Open-Loop Saturation and Goodput | All |
| [EXP-010](./EXP-010-Long-Context-Scaling-and-Limits.md) | Long-Context Scaling and Safe Limits | All; mandatory for capacity extension |
| [EXP-011](./EXP-011-Prefix-Cache-Efficacy-and-Isolation.md) | Prefix/KV Cache Efficacy, Eviction, and Isolation | All when caching enabled |
| [EXP-012](./EXP-012-CPU-GPU-and-Resource-Balance.md) | CPU/GPU Utilization and Resource Balance | All |
| [EXP-013](./EXP-013-Power-Thermals-and-Energy.md) | Power, Thermals, and Energy Efficiency | All |
| [EXP-014](./EXP-014-Disk-IO-and-Amplification.md) | Disk I/O and Amplification | All |
| [EXP-015](./EXP-015-Output-Correctness-and-Quality.md) | Output Correctness, Protocol Integrity, and Task Quality | All |
| [EXP-016](./EXP-016-Network-Fault-Injection.md) | Network and USB4 Fault Injection | Availability recovery; mandatory stable for dual operation |
| [EXP-017](./EXP-017-Process-and-Node-Fault-Injection.md) | Process and Node Fault Injection | Availability recovery; mandatory stable |
| [EXP-018](./EXP-018-Recovery-Rejoin-and-Cycle-Stability.md) | Recovery, Rejoin, and Cycle Stability | Availability recovery; all stable dual releases |
| [EXP-019](./EXP-019-Matched-Single-Node-Baselines-and-Scaling.md) | Matched Single-Node Baselines and Scaling Analysis | All; mandatory and nonwaivable |
| [EXP-020](./EXP-020-Soak-Reproduction-and-Stable-Proof.md) | Soak, Reproduction, and Stable Proof | All |

## Required sequence

`EXP-001 → EXP-002/003/004 → EXP-005–015 → EXP-016–018 → EXP-019 → EXP-020`.

Single-node controls begin after lab readiness and must be collected before a dual-node speedup claim. Fault tests begin only after fault-free correctness and recovery controls exist. Stable proof requires all mandatory cards and a signed decision against [`schemas/release-decision.schema.json`](../schemas/release-decision.schema.json).
