# Dated decision record: keep excluded

**Decision ID:** PF-IR-11  
**Date:** 2026-07-18  
**Priority:** P2  
**Status:** final for this research pass

## Decision

[DECISION] Keep the Ryzen AI MAX+ 395 XDNA2 NPU excluded from the production architecture, default builds, dependency graph, release packaging, and performance roadmap.

[DECISION] Continue the primary HIP/Vulkan fork without an XDNA2 workstream.

## What this decision does not deny

[UPSTREAM] The kernel driver and firmware substrate have reached a meaningful upstream state.

[VENDOR-ONLY] AMD provides a supported Linux package path and examples for some CNN, NLP, and LLM workloads.

[INFERENCE] These facts justify preserving a re-entry test, but they do not justify product integration.

## Blocking conditions

1. [MISSING] The target Linux installation is unknown.
2. [VENDOR-ONLY] The supported high-level path is versioned and package-coupled.
3. [UPSTREAM] IOMMU/SVA/PASID and native-host constraints are hard prerequisites.
4. [UPSTREAM] Memory is non-coherent; explicit cache maintenance and DMA movement remain in the critical path.
5. [MISSING] No target graph-assignment report demonstrates that the expensive classifier operators remain on NPU.
6. [MISSING] No target correctness, latency, energy, stability, suspend/resume, or recovery evidence exists.
7. [UNSUPPORTED] No generic `llama.cpp` NPU transformer path exists in the captured support boundary.
8. [UNKNOWN] Small OGA model availability does not establish a speculative-decoding interface or useful draft-model economics.
9. [UNKNOWN] The public embedding example does not establish a qualified Linux deployment for the target stack.
10. [MISSING] No exact Linux reranker example was captured.

## Re-entry authority

[DECISION] The only pre-authorized re-entry work is the isolated classifier experiment specified in [`bounded-classifier-experiment.md`](bounded-classifier-experiment.md). It is not a commitment to integrate.

## Review trigger

Re-open only when all of the following are available:

- a read-only target probe with the exact PCI revision, kernel config, driver source/version, firmware symlinks and hashes, package inventory, and IOMMU binding;
- reproducible AMD package hashes and license acceptance records;
- an exact task-specific ONNX model and compile/cache manifest;
- provider assignment showing the intended heavy subgraph on NPU;
- measured correctness, latency, energy, stability, and recovery against a CPU baseline;
- proof that the experiment remains isolated from HIP/Vulkan deliverables.
