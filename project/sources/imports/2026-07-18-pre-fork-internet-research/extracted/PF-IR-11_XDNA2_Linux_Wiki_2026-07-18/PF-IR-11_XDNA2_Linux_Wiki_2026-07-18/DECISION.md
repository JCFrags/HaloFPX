# PF-IR-11 decision

> **[DECISION] KEEP EXCLUDED — 2026-07-18**

The Ryzen AI MAX+ 395 XDNA2 NPU remains excluded from the product architecture and from the primary HIP/Vulkan fork path.

## Basis

[UPSTREAM] The Linux substrate is real: the pinned mainline `amdxdna` driver recognizes PCI `1022:17f0` revision `0x11`, the matching `linux-firmware` namespace exists, the UAPI exposes contexts, buffer objects, command submission, explicit synchronization, telemetry, firmware queries, and power/resource data.

[VENDOR-ONLY] A usable high-level model path still depends on a matched AMD package set: XRT base/NPU packages, the `amdxdna` plugin/shim, Ryzen AI Software, compiler passes, execution providers, and model/cache artifacts. AMD documents strict version coupling and warns that driver/firmware or driver/plugin mismatches can produce mailbox failures, aborted commands, or unsupported ioctls.

[UPSTREAM] The NPU is not cache coherent. Host DDR to local memory-tile/L2 movement is explicit and compiler/`ctrlcode` driven. AMD IOMMU, SVA/PASID, native execution, firmware loading, and locked-memory limits are material deployment constraints.

[MISSING] The actual target distro and installed stack have not been probed. There are no local measurements for correctness, graph partitioning, latency, throughput, energy, thermal behavior, memory movement, suspend/resume, or recovery.

[UNSUPPORTED] No `llama.cpp` NPU backend is established. The documented AMD `llama.cpp` path is for the iGPU.

## Permitted re-entry gate

[DECISION] One isolated auxiliary experiment may be considered only after the read-only probe passes:

- task: prompt classification or moderation represented by a task-specific DistilBERT-class classifier;
- model boundary: ONNX opset 17, BF16, batch 1, fixed sequence length 128;
- runtime boundary: AMD's matched Linux package set in an isolated environment;
- architectural boundary: no dependency from the primary inference path and no modifications to the HIP/Vulkan fork;
- acceptance boundary: correctness parity, demonstrated NPU placement, stable execution, and a material latency or energy benefit.

Failure of any gate retains `keep excluded`.

See [`decision/keep-excluded-2026-07-18.md`](decision/keep-excluded-2026-07-18.md) and [`decision/bounded-classifier-experiment.md`](decision/bounded-classifier-experiment.md).
