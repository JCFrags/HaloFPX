# 11. Tightly bounded auxiliary experiment

> This page summarizes the normative gate in [`../decision/bounded-classifier-experiment.md`](../decision/bounded-classifier-experiment.md).

## Experiment identity

[DECISION] One process, one task-specific DistilBERT-class classifier, ONNX opset 17, BF16, batch 1, fixed sequence length 128.

[DECISION] The CPU reference uses the same ONNX graph, tokenizer, inputs, outputs, evaluation corpus, and thresholding logic.

[DECISION] The NPU environment is isolated from the product build and from the HIP/Vulkan fork.

## Required sequence

1. Run the read-only probe.
2. Confirm exact PCI revision, IOMMU group, kernel config, module, firmware, and packages.
3. Capture package hashes and license acceptance before installation.
4. Create an isolated environment.
5. Produce and hash the exact ONNX graph and compile/cache outputs.
6. Capture provider/operator assignment.
7. Run correctness before performance.
8. Run warm-cache stability and performance tests.
9. Remove the environment and verify the primary path is unchanged.

## Pass criteria

| Gate | Requirement |
|---|---|
| Correctness | within 0.5 percentage point of CPU reference |
| Placement | expensive encoder subgraph on NPU |
| Stability | 10,000 requests without device/kernel/runtime failure |
| Performance | P95 latency ≥20% better, or energy/request ≥15% lower at equal/better P95 |
| Accounting | cold compile, cache load, sync, preprocessing, inference, and postprocessing separated |
| Isolation | no primary-path dependency or release-package change |
| Restart | ordinary process stop/start works without module reload or reboot |

## Fail-fast conditions

[DECISION] Stop and retain `keep excluded` on:

- firmware or plugin mixing;
- mailbox timeout or command abort;
- unsupported ioctl;
- device disappearance;
- mostly-CPU graph partition;
- accuracy regression;
- no material latency or energy gain;
- hidden install steps;
- any pressure to change the HIP/Vulkan roadmap.

## Claims prohibited after a narrow pass

Even a passing classifier experiment would not establish:

- embedding support;
- reranking support;
- LLM or `llama.cpp` offload;
- draft-model integration;
- coherent shared memory;
- multi-tenant QoS;
- suspend/resume transparency;
- VM support;
- general operator coverage;
- product readiness.
