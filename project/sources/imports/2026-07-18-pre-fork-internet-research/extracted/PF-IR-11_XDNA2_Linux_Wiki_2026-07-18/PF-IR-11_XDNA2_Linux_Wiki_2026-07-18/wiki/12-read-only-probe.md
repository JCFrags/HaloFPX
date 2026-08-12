# 12. Minimal non-invasive local probe

Script: [`../probe/xdna2_readonly_probe.sh`](../probe/xdna2_readonly_probe.sh)

## Safety boundary

The default probe:

- does not use `sudo`;
- does not install packages;
- does not load, unload, bind, or unbind modules;
- does not write sysfs or debugfs;
- does not reset the NPU;
- does not change IOMMU or kernel parameters;
- does not change locked-memory limits;
- does not suspend the system;
- does not compile or execute a model;
- does not open a workload context through XRT.

An optional `--xrt-query` flag runs only `xrt-smi examine` when already installed. It is disabled by default.

## Invocation

```bash
chmod +x probe/xdna2_readonly_probe.sh
./probe/xdna2_readonly_probe.sh
```

Specify a destination:

```bash
./probe/xdna2_readonly_probe.sh --output ./pf-ir-11-probe
```

Optional pre-existing XRT query:

```bash
./probe/xdna2_readonly_probe.sh --output ./pf-ir-11-probe --xrt-query
```

## Captured evidence

- OS release and kernel version;
- sanitized kernel command line;
- relevant kernel configuration symbols;
- PCI identity and revision;
- bound driver and IOMMU group;
- `modinfo`, loaded module state, and module parameters;
- `/dev/accel` nodes and permissions;
- filtered kernel log, when readable without privilege;
- exact firmware symlinks, metadata, and hashes;
- installed packages across `dpkg`, RPM, or Pacman;
- relevant runtime libraries;
- locked-memory limit;
- IOMMU group inventory;
- optional `xrt-smi examine`;
- `summary.json` and `SHA256SUMS`.

## Interpretation

A probe can establish substrate state, not model suitability.

| Probe result | Meaning |
|---|---|
| no `1022:17f0` | wrong device visibility or platform |
| revision not `0x11` | this npu5 conclusion does not directly apply |
| no IOMMU group | upstream npu5 path should fail |
| no `amdxdna` binding | kernel/device setup incomplete |
| missing firmware | boot should fail |
| firmware present but mismatched package lineage | unsafe to proceed |
| old distro module with new plugin | UAPI mismatch risk |
| substrate passes | experiment may be prepared; no performance claim |

## Privacy

The script sanitizes common `root=`, `resume=`, LUKS, and crypt-device command-line identifiers. Review the output before sharing because hostnames, package inventories, paths, and device topology can still be sensitive.
