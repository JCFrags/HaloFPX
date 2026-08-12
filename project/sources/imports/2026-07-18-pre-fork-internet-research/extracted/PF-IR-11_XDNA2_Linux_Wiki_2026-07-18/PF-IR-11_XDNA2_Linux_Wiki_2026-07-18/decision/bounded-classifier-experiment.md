# Conditional experiment: bounded auxiliary classifier

This document is a gate specification, not an instruction to integrate the NPU.

## Scope

[DECISION] The only admissible experiment is a small prompt-classification or moderation workload with the shape of AMD's public BF16 DistilBERT example.

| Parameter | Fixed boundary |
|---|---|
| Task | Prompt classification or moderation |
| Model family | DistilBERT-class encoder classifier |
| Interchange | ONNX, opset 17 |
| Numeric format | BF16 |
| Batch | 1 |
| Sequence length | 128, fixed |
| Runtime | `VitisAIExecutionProvider` from a matched AMD Linux package set |
| Deployment | isolated environment, isolated process, no primary-path dependency |
| Baseline | CPU ONNX Runtime using the same model inputs and outputs |

[INFERENCE] Moderation is classifier-shaped, but AMD's sentiment example is not a moderation model. A task-specific dataset, labels, policy threshold, and calibration are mandatory.

## Preconditions

1. [TARGET-DISTRO] The read-only probe identifies PCI `1022:17f0` revision `0x11` and a bound `amdxdna` driver.
2. [TARGET-DISTRO] Required kernel symbols are enabled and an IOMMU group is present.
3. [TARGET-DISTRO] Firmware links resolve under `amdnpu/17f0_11/` and hashes are recorded.
4. [TARGET-DISTRO] XRT, plugin, Ryzen AI, firmware, and driver versions are mutually matched.
5. [VENDOR-ONLY] Package filenames, hashes, source URLs, license/EULA records, and install commands are captured before installation.
6. [DECISION] The experiment has a separate environment, repository branch, build target, and removal procedure.
7. [DECISION] No HIP/Vulkan code or release dependency is changed.

## Required evidence

- exact model and tokenizer identifiers, licenses, SHA-256 hashes, and conversion command;
- exact ONNX opset, static input shapes, input/output dtypes, and graph hash;
- quantization/compilation command, configuration file, package versions, generated cache contents, and hashes;
- ONNX Runtime provider list and provider options;
- graph/operator assignment report distinguishing NPU and CPU nodes;
- CPU and NPU raw outputs for the same evaluation corpus;
- latency distributions for preprocessing, inference, synchronization, and postprocessing separately;
- host CPU utilization, NPU power/utilization telemetry where available, and wall energy measurement method;
- kernel log delta, firmware errors, command aborts, timeouts, and process exit status;
- cold-compile and warm-cache results separately.

## Acceptance gates

The experiment passes only when every gate passes:

1. **Correctness:** classification metric is within 0.5 percentage point of the CPU reference, with no unexplained per-example divergence.
2. **Placement:** the expensive encoder subgraph is assigned to NPU; CPU work is limited to tokenization, unsupported trivial edges, and postprocessing. A silent mostly-CPU partition is a fail.
3. **Stability:** 10,000 inference requests complete without kernel error, mailbox timeout, command abort, device disappearance, process crash, or forced reload.
4. **Performance:** either warm-cache P95 end-to-end latency is at least 20% lower than the CPU baseline, or measured energy per request is at least 15% lower at equal or better P95 latency.
5. **Cold-start accounting:** first-run compilation and cache generation are reported separately and do not contaminate warm-path claims.
6. **Memory accounting:** locked memory, device/heap allocation, input/output transfer, explicit sync, and cache size are recorded.
7. **Operational isolation:** removing the experiment leaves the primary product build, packaging, runtime, and test matrix unchanged.
8. **Recovery:** ordinary process termination and restart succeed without reboot or driver reload. Suspend/resume and fault injection are outside the initial experiment and cannot be claimed as validated.

## Automatic fail conditions

- any need to patch the product's HIP/Vulkan path;
- unsupported or undocumented package mixing;
- unrecorded firmware replacement;
- hypervisor deployment;
- IOMMU/SVA/PASID failure;
- mostly-CPU graph execution;
- material accuracy regression;
- no material latency or energy gain;
- repeated mailbox, firmware, reset, or suspend defects;
- reliance on unpublished manual steps.

[DECISION] Any failed gate returns to `keep excluded`.
