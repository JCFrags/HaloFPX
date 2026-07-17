---
section_id: "23"
title: "Software compatibility procedures and checks"
status: "needs-machine-validation"
last_verified: "2026-07-16"
applies_to:
  repositories: ["HaloFPX", "ROCmFPX"]
  software_versions: ["Linux", "ROCm", "Mesa/RADV"]
  hardware_revisions: ["two project Strix Halo nodes"]
related_sections: ["13", "18", "19", "20", "37", "50", "70", "81"]
---

# Software compatibility procedures and checks

## 1. Immutable inventory

Run on both nodes into separate timestamped evidence directories. Commands are read-only; some require root.

```bash
date --iso-8601=seconds
uname -a
cat /etc/os-release
cat /proc/cmdline
zcat /proc/config.gz 2>/dev/null || cat /boot/config-"$(uname -r)"
modinfo amdgpu
grep . /sys/module/amdgpu/parameters/* 2>/dev/null
sudo lspci -Dnnk
sudo dmesg -T | grep -Ei 'amdgpu|kfd|firmware|drm|thunderbolt|usb4|iommu'
lsmod | grep -E 'amdgpu|thunderbolt'
```

Record package authorities and exact versions (`dpkg-query -W`, `rpm -qa`, or distribution equivalent) for kernel, firmware, ROCm/HIP/HSA, LLVM/Clang, Mesa, Vulkan loader/tools, and compiler dependencies.

## 2. Required KFD fix ancestry

For a source tree corresponding to the running kernel:

```bash
git merge-base --is-ancestor 7f26af7bf9b76c2c2a1a761aab5803e52be21eea HEAD
git merge-base --is-ancestor 7445db6a7d5a0242d8214582b480600b266cba9e HEAD
git show --no-patch --format=fuller HEAD
```

Distribution kernels may carry rebased backports, so failed ancestry does not prove absence. In that case link the distribution source changelog/patch and verify the corresponding source changes; record as backported, not upstream ancestry.

## 3. Firmware evidence

From the boot log, list every requested/loaded amdgpu firmware filename. Then capture package version and hashes:

```bash
find /usr/lib/firmware /lib/firmware -path '*/amdgpu/*' -type f -print0 2>/dev/null | sort -z | xargs -0 sha256sum
fwupdmgr get-devices --json 2>/dev/null
fwupdmgr get-updates --json 2>/dev/null
```

Do not update firmware as part of inventory. Any update needs OEM applicability, preserved old manifest, power-safe procedure, and rollback/recovery plan.

## 4. HIP/HSA/compiler checks

```bash
rocminfo
rocm-smi --showdriverversion --showfwinfo 2>/dev/null || true
amd-smi version --json 2>/dev/null || true
hipconfig --full
amdclang++ --version 2>/dev/null || hipcc --version
```

Build and preserve source/output for a minimal device enumeration, allocation, kernel launch, result verification, and host-device copy at `--offload-arch=gfx1151`. Also run the repository's pinned smoke test. Record HSA agent name/features, compiler code-object target, errors, and kernel log.

## 5. Mesa/RADV/Vulkan checks

```bash
vulkaninfo --summary
vulkaninfo --json > vulkaninfo.json
glxinfo -B 2>/dev/null || true
ldconfig -p | grep -E 'libvulkan_radeon|libamdhip64|libhsa-runtime'
```

Record Mesa version, ICD JSON path, loaded shared objects (`LD_DEBUG=libs` in a bounded smoke run if needed), reported API, device/driver UUID, and extensions required by the pinned llama.cpp/Vulkan build. Do not promote the global Mesa API claim as a device result.

## 6. USB4 paths

Supported control:

```bash
sudo modprobe thunderbolt-net
ip -details link show type thunderbolt
find /sys/bus/thunderbolt/devices -maxdepth 3 -type f -readable -print -exec cat {} \; 2>/dev/null
```

Experimental Linux 7.2 lane only:

```bash
grep -E 'CONFIG_(THUNDERBOLT|CONFIGFS_FS)' /boot/config-"$(uname -r)"
sudo modprobe thunderbolt-stream
find /sys/kernel/config/thunderbolt/stream -maxdepth 3 -print
ls -l /dev/tbstream*
```

Follow the upstream ConfigFS procedure in [S23-07] and preserve the exact stream/domain/route mapping. Test each physical link alone, then both, then coexistence with `thunderbolt-net`, and inspect kernel errors. Root writes to ConfigFS require an approved test window and cleanup.

## 7. Qualification ladder

1. cold boot and two warm reboots;
2. firmware/amdgpu/KFD clean enumeration;
3. HIP/HSA and Vulkan independent smoke tests;
4. single-node deterministic inference;
5. 60-minute single-node mixed load;
6. each link independently, then dual link;
7. 60-minute distributed inference plus cache I/O;
8. service restart, cable loss/recovery, and supported suspend/resume;
9. boot known-good rollback entry.

No combination becomes `[MEASURED]` or production-verified without raw logs, exact versions/commits/hashes, environment, and pass/fail assertions.

[S23-07]: sources.md#s23-07
