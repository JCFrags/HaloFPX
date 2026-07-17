# SLO Definition and Approval

> **Wiki status:** Proposed · **Evidence state:** D0 — design only · **Last reviewed:** 2026-07-17  
> **Machine-validation status:** Not run. Missing measurements are `INSUFFICIENT_EVIDENCE`, never an implicit pass.



Absolute SLOs are product requirements, not values that can be inferred from a benchmark result. Because the target model, quantization, context distribution, concurrency, and caller experience are not specified in this design bundle, TTFT/ITL numbers remain a mandatory deployment input. This is an explicit release block, not permission to omit them.

## Approval sequence

1. Select the release profile and exact workload cells.
2. Set p95/p99 client TTFT, p95/p99 ITL, success, correctness, throughput/goodput where applicable, and recovery bounds.
3. Record the rationale, owner, and approval timestamp in [`templates/slo-approval.yaml`](../templates/slo-approval.yaml) and `config/sut.yaml`.
4. Freeze the SLO artifact before the first M1 run and hash it into every run manifest.
5. Treat any later relaxation as a new claim/profile lineage; do not retrofit the old candidate.

## Universal stable floors

| Gate | Default |
|---|---:|
| Request success | ≥99.99% |
| Critical correctness | 100% |
| Unexpected crash/hang/oops/GPU reset/silent corruption/thermal throttle/normal-run USB4 renegotiation | 0 |
| Lost acknowledged requests during mandatory faults | 0 |
| Fault detection | ≤10 s unless stricter approved SLO |
| Affected request reaches explicit terminal state | ≤30 s |
| Reversible service recovery | ≤120 s |

## Per-workload fields

Every production workload cell must state topology scope, cache state, concurrency/offered load, p95/p99 TTFT and ITL, success floor, correctness suite, and approver. A `null`, absent, or unapproved value produces `INSUFFICIENT_EVIDENCE`. Relative speedup or regression performance cannot override an absolute SLO failure.

The machine-readable policy is [`config/slo-policy.yaml`](../config/slo-policy.yaml).
