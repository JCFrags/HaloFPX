# 1. Scope and baseline

## Question

Determine whether the Ryzen AI MAX+ 395 XDNA2 NPU is suitable on Linux for a **bounded auxiliary role**: embeddings, reranking, moderation, prompt classification, or a small draft model.

This research intentionally does not evaluate the NPU as a primary transformer engine.

## Explicit non-assumptions

[UNSUPPORTED] No `llama.cpp` NPU offload is assumed. AMD's captured framework overview associates `llama.cpp` with the iGPU, not the NPU.

[UPSTREAM] No coherent shared-memory model is assumed. The driver explicitly says the NPU is not cache coherent and supplies an explicit synchronization ioctl.

[UNKNOWN] No useful end-to-end performance is assumed. A model loading successfully is not evidence of useful latency, throughput, energy, stability, or operator placement.

## Hardware applicability

[INFERENCE] Ryzen AI MAX+ 395 / Strix Halo corresponds to the upstream `npu5` device entry:

| Field | Expected value | Evidence |
|---|---|---|
| PCI vendor | `0x1022` | AMD |
| PCI device | `0x17f0` | upstream `pci_ids[]` |
| revision | `0x11` | upstream `amdxdna_ids[]` |
| device info | `dev_npu5_info` | upstream mapping |
| VBNV | `RyzenAI-npu5` | upstream npu5 definition |
| queue type | KMQ | upstream npu5 definition |
| context limit | 16 | upstream npu5 definition |
| firmware directory | `amdnpu/17f0_11/` | upstream npu5 definition |

Local evidence:
- [`../sources/raw/kernel/npu5_regs.c`](../sources/raw/kernel/npu5_regs.c)
- [`../sources/excerpts/kernel/amdxdna_pci_drv.c.lines26-139.txt`](../sources/excerpts/kernel/amdxdna_pci_drv.c.lines26-139.txt)

## Reference Linux baseline versus target Linux

[VENDOR-ONLY] AMD's captured Ryzen AI Software 1.7.1 Linux reference is Ubuntu 24.04 LTS, kernel 6.10 or newer, Python 3.12.x, and exact versioned package filenames.

[MISSING] The actual target distro was not supplied. This bundle therefore reports two separate states:

1. **Documented reference state:** what AMD and upstream sources claim.
2. **Target state:** unresolved until the read-only probe is run.

[TARGET-DISTRO] A distro kernel may recognize the PCI device yet be too old for the userspace plugin's current ioctls. Enumeration alone is not acceptance.

## Evidence method

Primary evidence is pinned by commit/version and access date in [`../manifests/sources.csv`](../manifests/sources.csv). Literal claim labels are defined in [`../CLAIM_LABELS.md`](../CLAIM_LABELS.md). Every included file is hashed in [`../manifests/files.sha256`](../manifests/files.sha256).
