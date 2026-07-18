# 6. Memory, DMA, and IOMMU boundary

## Coherence

[UPSTREAM] The upstream driver explicitly states that the NPU is **not cache coherent**.

[UPSTREAM] The UAPI exposes `SYNC_DIRECT_TO_DEVICE` and `SYNC_DIRECT_FROM_DEVICE`, and the driver performs explicit cache flushing/invalidation outside command submission.

Exact source excerpt: [`../sources/excerpts/kernel/amdxdna_gem.c.noncoherent-sync.txt`](../sources/excerpts/kernel/amdxdna_gem.c.noncoherent-sync.txt).

[DECISION] Do not describe the platform as shared coherent memory for NPU workloads.

## Data movement

[UPSTREAM] The architecture has per-column DMA engines that move data between host DDR and memory-tile/L2 storage.

[UPSTREAM] Local L2 is software managed.

[UPSTREAM] DMA instructions are encoded in compiler-generated `ctrlcode`; execution moves data while the array runs.

[INFERENCE] A host pointer visible through SVA is not equivalent to zero-copy coherent execution. Page mapping, pinning, cache maintenance, DMA, local placement, and synchronization remain material costs.

## Buffer model

[UPSTREAM] The UAPI includes:

- shared host buffers;
- device heap buffers;
- device buffers allocated from a heap;
- command buffers;
- DMA-BUF or virtual-address table inputs;
- explicit mapping and synchronization information.

[UPSTREAM] Each workload context uses a host-resident instruction buffer described by the kernel documentation as 64 MB.

[VENDOR-ONLY] AMD's source-build guidance notes that locked-memory limits can prevent large buffer allocation and may need configuration.

[DECISION] The read-only probe records the current locked-memory limit but does not change it.

## IOMMU, SVA, HMM, and PASID

[UPSTREAM] `amdxdna` requires AMD IOMMU support at Kconfig level.

[UPSTREAM] The npu5 path requires an IOMMU group and normally binds SVA per client, obtaining a PASID.

[UPSTREAM] The driver uses HMM/MMU interval-notifier mechanisms to repopulate invalidated user mappings before command submission.

[UPSTREAM] PASID is also part of hardware context isolation and partition access control.

[INFERENCE] IOMMU availability is a functional dependency, not an optional performance feature.

## Virtualization boundary

[UPSTREAM] The npu5/AIE2 initialization code rejects a non-native hypervisor environment.

[UNSUPPORTED] This research does not approve VM passthrough, mediated devices, or nested/container abstractions as deployment targets.

## Measurement requirements

A bounded experiment must separate:

- tokenization and input construction;
- host allocation and pinning;
- compile/cache load;
- host-to-device synchronization;
- NPU execution;
- device-to-host synchronization;
- output postprocessing.

Reporting only `session.run()` or only array execution time is insufficient for the auxiliary-role decision.
