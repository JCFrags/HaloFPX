# Validation Coverage Matrix

> **Wiki status:** Proposed · **Evidence state:** D0 — design only · **Last reviewed:** 2026-07-17  
> **Machine-validation status:** Not run. Missing measurements are `INSUFFICIENT_EVIDENCE`, never an implicit pass.



| Required dimension | Primary cards | Release evidence |
|---|---|---|
| Power-on cold / OS-warm / model-warm / exact-prefix warm | EXP-003, EXP-004, EXP-011 | C0–C3 state proof, load/TTFT/read/cache deltas |
| Prefill and decode | EXP-005, EXP-006 | Prompt and generation rates, paired scaling |
| TTFT and inter-token latency | EXP-007 | Client p50/p95/p99 plus decomposition |
| Throughput and saturation | EXP-008, EXP-009 | Goodput, knee, admission, headroom |
| Long context | EXP-010 | Correct supported limit, slopes, safe failure |
| Cache hit rates | EXP-004, EXP-011 | Eligible token/request hit, false-hit/isolation |
| Disk amplification | EXP-003, EXP-004, EXP-014 | Cold/warm reads and sustained writes |
| USB4 utilization | EXP-002, EXP-005–009, EXP-016 | Measured link reference and inference fraction |
| CPU/GPU utilization | EXP-012 | Aligned utilization, pressure, participation |
| Power and thermals | EXP-013, EXP-020 | Wall energy, margin, throttles, soak |
| Output correctness | EXP-015 plus canaries in every card | Critical correctness and task drift |
| Fault injection/recovery | EXP-016–018 | Network/process/node faults, rejoin cycles |
| Matched single-node baselines | EXP-019 | A, B, dual paired ratios and claim boundary |
| Stable duration/reproduction | EXP-020 | 24 h RC, 72 h R1, freshness and signed decision |

No row can be satisfied by a page existing. Evidence status is determined from run manifests, raw files, and gate evaluation.
