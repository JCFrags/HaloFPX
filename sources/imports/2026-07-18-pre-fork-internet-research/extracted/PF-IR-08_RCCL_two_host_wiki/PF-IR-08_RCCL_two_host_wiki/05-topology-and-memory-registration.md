# Topology discovery and memory registration

## Topology discovery

RCCL builds a node-local graph from GPU, PCI, CPU, NIC, and network nodes. A Net plugin reports `pciPath`, speed, latency, GUID, pointer support, and device-offload properties. A physical or virtual USB network device may expose useful sysfs ancestry or may yield no PCI path.

`[INFERENCE]` RCCL can score the local Linux-visible NIC relationship, but it does not infer the remote physical USB4 cable internals from an IP interface name. Capture topology XML/logs on both hosts instead of assuming symmetry.

## Public buffer registration versus effective path

In 2.27.7 the `ncclCommRegister` API is present, yet `NCCL_LOCAL_REGISTER` defaults to zero with a source comment saying the feature is off by default for RCCL as unsupported. With the default, the implementation returns a null handle. Public API presence is therefore not evidence that an optimized local registration path is active.

## DMA-BUF boundary

The core contains conditional ROCr DMA-BUF export plumbing. It checks an environment gate, ROCr capability, the `hsa_amd_portable_export_dmabuf` symbol, and kernel features. A Net provider must still advertise DMA-BUF support and implement `regMrDmaBuf`. Stock Socket does neither.

Consequently, `NCCL_DMABUF_ENABLE=1` in a log only shows an attempted core feature gate. It does not identify the selected provider, successful registration, zero-copy behavior, or GPU-direct transfer.
