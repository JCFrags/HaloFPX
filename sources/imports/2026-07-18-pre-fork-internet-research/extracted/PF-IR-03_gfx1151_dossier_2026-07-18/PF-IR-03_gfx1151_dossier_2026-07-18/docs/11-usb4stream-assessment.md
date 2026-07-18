# Linux 7.2 USB4STREAM assessment

## Captured upstream state

At the 2026-07-18 capture:

- stable Linux: 7.1.3;
- mainline: 7.2-rc3;
- linux-next: next-20260717.

The linux-next admin guide documents direct cable streaming through `thunderbolt-stream`, ConfigFS at `/sys/kernel/config/thunderbolt/stream`, and `/dev/tbstreamX`. Multiple streams and simultaneous `thunderbolt-net` use are described.

## Decision

[PLAUSIBLE_SEPARATE_CANDIDATE] A Linux 7.2/USB4STREAM lane is technically plausible for the two-node use case. [UNVERIFIED_COMBINATION] It is not an AMD-qualified gfx1151 ROCm tuple in the captured matrix and Linux 7.2 was not stable.

## Baseline-preserving test design

1. Freeze either the 7.2 control userspace or the 7.14 candidate userspace unchanged.
2. Add a separate boot entry containing an exact 7.2-rc tag/commit or exact linux-next commit with `CONFIG_USB4_STREAM` and its ConfigFS dependency.
3. Retain signed kernel source provenance, `.config`, patch list and build toolchain.
4. Confirm IOMMU/DMA protection and Thunderbolt security level.
5. Run the entire gfx1151 compute regression before attributing any failure to USB4STREAM.
6. Run two-node stream correctness, sustained throughput, backpressure, hotplug, teardown, suspend/resume and simultaneous `thunderbolt-net` tests.
7. Keep USB4STREAM transport results separate from RCCL transport claims unless RCCL is explicitly integrated with that interface and source-pinned.

## Preflight

`scripts/usb4stream-preflight.sh` checks kernel/config/module/ConfigFS/IOMMU evidence without creating or deleting streams. The actual stream test remains local because topology and peer identifiers are machine-specific.
