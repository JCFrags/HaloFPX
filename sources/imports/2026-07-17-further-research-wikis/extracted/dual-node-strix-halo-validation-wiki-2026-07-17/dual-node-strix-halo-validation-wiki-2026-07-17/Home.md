# Home — Dual-Node Validation Program

> **Wiki status:** Proposed · **Evidence state:** D0 — design only · **Last reviewed:** 2026-07-17  
> **Machine-validation status:** Not run. Missing measurements are `INSUFFICIENT_EVIDENCE`, never an implicit pass.


> **Program decision:** Stable release requires measured, reproducible evidence from both matched single-node baselines and the dual-node SUT. Documentation completion is classified as `DESIGN_COMPLETE`, not `MACHINE_VALIDATED`.

## Mission

Demonstrate that the dual-node system is correct, observable, reproducible, performant for its declared operating profile, resilient to bounded faults, and supportable against upstream change. The program measures cold and warm starts, prefill, decode, TTFT, inter-token latency, throughput, long-context scaling, cache efficacy, disk and USB4 behavior, utilization, power, thermals, correctness, recovery, and single-node deltas.

## Release profiles

| Profile | Legitimate claim | Required comparison |
|---|---|---|
| `scale_out` | Two nodes improve goodput or latency for a workload that fits on one node | Same model, quantization, context, prompts, build, and SLO on Node A, Node B, and dual-node |
| `capacity_extension` | Two nodes enable a model/context that cannot safely fit on one node | Absolute SLO plus a smaller same-family control workload that fits on each node; no speedup claim |
| `availability_recovery` | The pair detects faults and returns to service within declared bounds | Fault-free control, repeated injected faults, and post-recovery equivalence |

## Evidence ladder

`D0 Design` → `S0 Synthetic tool check` → `M1 Single-node measured` → `M2 Dual-node measured` → `R1 Reproduced` → `STABLE Accepted`.

A release cannot skip `M1`, because dual-node figures without matched single-node controls do not identify network or partitioning overhead.

## Navigation

- [Program charter](wiki/Program-Charter.md)
- [System under test](wiki/System-Under-Test.md)
- [Benchmark methodology](wiki/Benchmark-Methodology.md)
- [Metric definitions](wiki/Metric-Definitions.md)
- [Validation coverage matrix](wiki/Validation-Coverage-Matrix.md)
- [Release gates](wiki/Release-Gates.md)
- [SLO definition and approval](wiki/SLO-Definition-and-Approval.md)
- [Experiment index](experiments/README.md)
- [Upstream watch](wiki/Upstream-Watch.md) and [query catalog](wiki/Upstream-Query-Catalog.md)
- [Implementation roadmap](wiki/Implementation-Roadmap.md)
- [Current evidence status](EVIDENCE-STATUS.md)
