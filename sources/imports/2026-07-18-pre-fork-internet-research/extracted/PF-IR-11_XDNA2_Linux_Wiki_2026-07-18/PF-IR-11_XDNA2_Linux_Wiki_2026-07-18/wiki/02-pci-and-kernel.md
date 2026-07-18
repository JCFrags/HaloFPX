# 2. PCI and upstream kernel boundary

## Mainline status

[UPSTREAM] `amdxdna` is present in the pinned Linux tree at commit `94515f3a7d4256a5062176b7d6ed0471938cd51a`.

[UPSTREAM] The initial mainline driver commit is pinned as `8c9ff1b181ba3d31d6b4a48606248b52180a7046`. Its initial scope was basic device initialization, firmware loading, power-up, and low-level initialization; the current pinned tree has materially broader UAPI and runtime behavior.

## Exact PCI applicability

[UPSTREAM] The current mapping includes:

| PCI device | revision | driver device |
|---|---:|---|
| `0x17f0` | `0x10` | npu4 |
| `0x17f0` | `0x11` | **npu5** |
| `0x17f0` | `0x20` | npu6 |

[INFERENCE] The research target is applicable only when the real host reports `1022:17f0` revision `0x11`. A different revision must be re-evaluated against its own device entry.

## Kernel build boundary

[UPSTREAM] `CONFIG_DRM_ACCEL_AMDXDNA` depends on:

- `CONFIG_AMD_IOMMU`
- `CONFIG_DRM_ACCEL`
- PCI and I/O memory
- x86-64

It selects the DRM scheduler, GEM shared-memory helper, firmware loader, and HMM mirror support.

Exact source: [`../sources/raw/kernel/Kconfig`](../sources/raw/kernel/Kconfig).

## UAPI boundary

[UPSTREAM] The pinned driver advertises UAPI major/minor `0.10`. The captured UAPI includes:

- hardware context creation, configuration, destruction, and priorities;
- shared, device-heap, device, and command buffer objects;
- explicit buffer synchronization directions;
- command submission, dependencies, signals, waits, and sync objects;
- firmware, AIE metadata, clock, power, utilization, resource, context, and telemetry queries;
- context counters, preemptions, migrations, suspensions, errors, and heap usage.

Exact selected declarations: [`../sources/excerpts/kernel/amdxdna_accel.h.qos-bo-sync-telemetry.txt`](../sources/excerpts/kernel/amdxdna_accel.h.qos-bo-sync-telemetry.txt).

## IOMMU and native-host boundary

[UPSTREAM] The npu5/AIE2 initialization path rejects operation without an IOMMU group.

[UPSTREAM] The same path rejects operation under a hypervisor.

[UPSTREAM] A normal client open binds SVA and obtains a PASID. If PASID is unavailable and no supported carveout path is configured, open fails.

[INFERENCE] A container may be feasible only when it still runs on a native host with the required device and IOMMU interfaces passed through; this bundle does not claim container support.

## Upstream versus AMD vendor tree

[UPSTREAM] A distro may ship the in-tree driver.

[VENDOR-ONLY] AMD's `amd/xdna-driver` repository also builds a staging/upstream-style module and a separate legacy/OOT module, plus the XRT NPU shim and firmware packaging.

[VENDOR-ONLY] AMD warns that a newer plugin may issue ioctls that an older in-tree driver lacks. The correct comparison is therefore not “driver present” but “driver UAPI, firmware, shim, and XRT matched.”

## Target probe gate

[TARGET-DISTRO] Record all of the following before installing anything:

- `lspci -nn -D` vendor/device/revision;
- `modinfo amdxdna` filename, version, aliases, signer, and firmware declarations;
- `/sys/bus/pci/devices/.../driver` and `iommu_group`;
- active kernel config symbols;
- `/dev/accel` nodes and permissions;
- kernel log lines related to `amdxdna`, firmware, IOMMU, PASID, mailbox, PSP, and SMU.
