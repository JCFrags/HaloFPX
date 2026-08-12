# PF-IR-11: Linux XDNA2 auxiliary-role boundary

> **[DECISION] KEEP EXCLUDED — 2026-07-18**

The expected Ryzen AI MAX+ 395 XDNA2 NPU has a credible upstream Linux kernel and firmware substrate, but the target is not yet a reproducible, measured, product-ready auxiliary accelerator.

## What is established

[INFERENCE] The expected hardware identity is PCI `1022:17f0`, revision `0x11`, mapped by the pinned upstream driver to `npu5`.

[UPSTREAM] The driver exposes context creation, buffer objects, command submission, explicit buffer synchronization, firmware/version queries, telemetry, power/resource queries, scheduling, suspend/resume, and error reporting.

[UPSTREAM] Firmware names for this revision are declared as `amdnpu/17f0_11/npu.sbin` and `amdnpu/17f0_11/npu_7.sbin`.

[VENDOR-ONLY] The documented high-level Linux path uses AMD's exact XRT base/NPU packages, the XDNA plugin/shim, Ryzen AI Software, ONNX Runtime execution providers, compiler passes, and prepared model/cache artifacts.

## What is not established

[MISSING] The actual distro, kernel configuration, firmware package, loaded driver, package inventory, permissions, and IOMMU binding on the target.

[MISSING] Operator placement, correctness, latency, throughput, energy, memory movement, thermal behavior, suspend/resume transparency, and recovery on the target.

[UNSUPPORTED] A `llama.cpp` transformer backend for the NPU.

[UNKNOWN] A qualified Linux embedding or reranking path that is reproducible on the target.

[UNKNOWN] A useful speculative-decoding/draft-model interface, despite listed 135M OGA models.

## Decision boundary

[DECISION] Do not add XDNA2 dependencies, build jobs, release packages, abstractions, or roadmap work to the HIP/Vulkan fork.

[DECISION] Preserve only one optional re-entry gate: an isolated BF16 DistilBERT-class prompt classifier or moderation experiment after the read-only substrate probe.

## Start here

- [Scope and baseline](01-scope-and-baseline.md)
- [Exact kernel and PCI boundary](02-pci-and-kernel.md)
- [Firmware boundary](03-firmware.md)
- [Runtime and compiler boundary](04-runtime-and-compiler.md)
- [Role evaluation](10-role-evaluation.md)
- [Bounded experiment](11-bounded-experiment.md)
- [Read-only probe](12-read-only-probe.md)
- [Source index](14-source-index.md)
