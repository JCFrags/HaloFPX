---
section_id: "19"
title: "Unified-memory procedures and checks"
status: "needs-machine-validation"
last_verified: "2026-07-16"
applies_to:
  repositories: []
  software_versions: ["ROCm/HIP", "Linux amdgpu/TTM"]
  hardware_revisions: ["Radeon 8060S / gfx1151"]
related_sections: ["23", "24", "54", "74", "84"]
---

# Procedures and checks

All workload experiments require an idle gate, exact environment snapshot, cgroup scope, no production service, and raw synchronized telemetry. Do not change BIOS/kernel/IOMMU settings without a separate reversible experiment plan.

## S19-E01 — Read-only capability and accounting snapshot

```bash
uname -a; cat /proc/cmdline
free -b; cat /proc/meminfo; cat /proc/pressure/memory
cat /sys/module/ttm/parameters/pages_limit
cat /sys/module/ttm/parameters/page_pool_size
cat /sys/module/amdgpu/parameters/no_system_mem_limit
find /sys/class/drm/card*/device -maxdepth 1 -type f -name 'mem_info_*' -print -exec cat {} \;
rocminfo
rocm-smi --showmeminfo all
find /sys/kernel/iommu_groups -maxdepth 2 -type l -print
dmesg --level=warn,err | grep -Ei 'amdgpu|kfd|ttm|iommu|oom|fault'
```

Also query HIP attributes: managed memory, pageable access, concurrent managed access, host-register support, total/free memory, and allocation granularity.

## S19-E02 — Allocation staircase

For `hipMalloc`, `hipMallocManaged`, `hipHostMalloc`, and `hipHostRegister`, allocate/touch/free increasing sizes in a dedicated cgroup. Test one process, then two concurrent rank-like processes. Record the last clean size and first failure, API/error code, wall time, faults, RSS, TTM/GTT/VRAM, PSI, swap, and dmesg. Stop before OOM using a predeclared memory/PSI ceiling.

## S19-E03 — Bandwidth and coherence matrix

Measure CPU read/write, GPU sequential/random read/write, CPU↔GPU alternating access, and atomic/flag handoff for each allocator and coarse/fine policy. Verify data hashes. Report bandwidth/latency distributions, clocks, and energy; never infer from theoretical 256 GB/s.

## S19-E04 — Fault and migration characterization

Where attributes say supported, first-touch pages on CPU then GPU and reverse. Capture HMM/KFD/amdgpu faults and residency over time. If HMM/XNACK capability is absent, record `not supported` rather than forcing a test.

## S19-E05 — Contention and fragmentation

Run model-like weight allocation plus KV growth, pinned dual-rail buffers, CPU memory bandwidth load, and optional NVMe I/O in controlled cells. Randomize order and restore idle state. Determine safe admission reserve and whether long-lived fragmentation lowers the allocation ceiling.

## S19-E06 — Rebooted IOMMU A/B

Requires explicit approval and console recovery. Compare identical workloads with the platform-supported IOMMU policy versus current policy. Verify IOMMU groups, NPU probe, USB4 DMA protection, GPU functionality, allocation ceiling, bandwidth, faults, and rollback. Security/functionality is part of the verdict, not only speed.
