# Wiki contents

## Orientation

- [Home](docs/index.md)
- [Executive summary](docs/executive-summary.md)
- [How to read estimates](docs/methodology/evidence-labels.md)
- [Assumptions and evidence ledger](docs/assumptions-and-evidence.md)
- [Glossary](docs/glossary.md)

## Platform

- [Strix Halo platform facts](docs/hardware/strix-halo.md)
- [Dual USB4 topology](docs/interconnect/dual-usb4.md)
- [Transport and striping design](docs/interconnect/transport-and-striping.md)

## Cost model

- [Notation](docs/methodology/notation.md)
- [Network model](docs/cost-models/network.md)
- [Memory and KV model](docs/cost-models/memory.md)
- [Prefill formulas](docs/cost-models/prefill.md)
- [Decode formulas](docs/cost-models/decode.md)
- [Break-even methods](docs/cost-models/break-even.md)

## Execution modes

- [Tensor parallelism](docs/modes/tensor-parallel.md)
- [Contiguous layer splitting](docs/modes/contiguous-layer-split.md)
- [Pipeline parallelism](docs/modes/pipeline-parallel.md)
- [MoE expert placement](docs/modes/moe-expert-placement.md)
- [Remote speculation](docs/modes/remote-speculation.md)
- [Replicated decode](docs/modes/replicated-decode.md)
- [Hybrid approaches](docs/modes/hybrid.md)

## Placement and feasibility

- [Ownership matrix](docs/placement/ownership-matrix.md)
- [Placement schema guide](docs/placement/schema-guide.md)
- [Feasibility gates](docs/feasibility/gates.md)
- [Failure modes](docs/feasibility/failure-modes.md)
- [Go/no-go decision framework](docs/decision-framework.md)

## Worked examples

- [Worked-example method](docs/examples/index.md)
- [Llama 3.1 8B](docs/examples/llama31-8b.md)
- [Llama 3.1 405B](docs/examples/llama31-405b.md)
- [Mixtral 8x7B](docs/examples/mixtral-8x7b.md)
- [Qwen3-30B-A3B](docs/examples/qwen3-30b-a3b.md)

## Measurement and audit

- [Benchmark plan](docs/benchmarking/benchmark-plan.md)
- [Measurement worksheet](docs/benchmarking/measurement-worksheet.md)
- [Implementation notes](docs/implementation-notes.md)
- [Limitations](docs/limitations.md)
- [Sources](docs/sources.md)
- [Reproducibility audit](AUDIT.md)
