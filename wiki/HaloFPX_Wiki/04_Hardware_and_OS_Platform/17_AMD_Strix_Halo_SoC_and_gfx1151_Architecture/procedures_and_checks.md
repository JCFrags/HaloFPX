---
section_id: "17"
title: "Strix Halo procedures and checks"
status: "needs-machine-validation"
last_verified: "2026-07-16"
applies_to:
  repositories: []
  software_versions: []
  hardware_revisions: ["Two project Strix Halo machines; exact revisions OPEN"]
related_sections: ["18", "19", "20", "22", "23", "24", "25", "37", "74"]
---

# Procedures and checks

Run on **both** machines. These are read-only unless a build directory is explicitly created. Save stdout/stderr, UTC timestamp, hostname, kernel, firmware, package versions, and command exit code under the experiment record owned by the relevant section. No result below is pre-populated as **[MEASURED]**.

## Prerequisites

- Linux shell with `lscpu`, `find`, `sort`, `lspci`, `uname`, `dmesg`, and `journalctl`.
- ROCm tools (`rocminfo`, `rocm_agent_enumerator`, `amd-smi`) for HIP/HSA checks.
- Vulkan tools (`vulkaninfo`) and Mesa package metadata for RADV checks.
- Root: **not required** for topology and normal runtime queries; **may be required** to read the full kernel log. Do not change BIOS, kernel parameters, clocks, or power limits in this section.

## S17-EXP-001 — CPU cache and complex topology

```bash
date -u +%FT%TZ
hostnamectl
uname -a
lscpu --extended=CPU,NODE,SOCKET,CORE,CACHE,ONLINE,MAXMHZ,MINMHZ
lscpu --caches
find /sys/devices/system/cpu/cpu0/cache -maxdepth 2 -type f -print -exec sh -c 'printf "="; cat "$1"' sh {} \;
```

Pass evidence: all online logical CPUs map to stable core/socket/node/cache IDs and cache sizes agree with `lscpu`. **[OPEN]** Interpret CCD/CCX boundaries only where Linux exposes defensible shared-cache topology; do not infer them from CPU numbering alone.

## S17-EXP-002 — HSA identity, pools, and firmware carveout

```bash
date -u +%FT%TZ
lspci -nnk | grep -A4 -Ei 'VGA|Display|AMD'
rocminfo
rocm_agent_enumerator -name
amd-smi static --asic --board --driver --vram --partition
find /sys/class/drm/card*/device -maxdepth 1 -type f \
  \( -name 'mem_info_*' -o -name 'gpu_busy_percent' -o -name 'current_link_*' \) \
  -print -exec sh -c 'printf "="; cat "$1"' sh {} \;
```

Pass evidence: one intended GPU agent reports `gfx1151`; CU count and cache/pool fields are captured; physical RAM, firmware VRAM/carveout, and backend-allocatable pools are explicitly distinguished. Tool absence or permission errors are recorded, not worked around with privileged mutation.

## S17-EXP-003 — HIP target and datatype correctness gate

Prerequisite: exact source commit and clean disposable build directory for the selected microtest/engine. Root: no.

```bash
hipcc --version
rocminfo | grep -E 'Name:|Marketing Name:|Wavefront Size|Compute Unit|Cache Info' 
rocm_agent_enumerator -name
```

Build the project's minimal GEMM/WMMA correctness fixture with an explicit target (example build-system flag):

```bash
cmake -S experiments/gfx1151-datatypes -B build/s17-gfx1151 \
  -DCMAKE_BUILD_TYPE=Release -DAMDGPU_TARGETS=gfx1151
cmake --build build/s17-gfx1151 --verbose
ctest --test-dir build/s17-gfx1151 --output-on-failure
```

Required fixture matrix: FP32 reference; FP16 and BF16; INT8 and INT4 packing; WMMA enabled/disabled variants; deterministic seeds; CPU reference; absolute/relative error criteria recorded per datatype. Pass requires native `gfx1151` compilation, zero runtime errors, and all correctness thresholds. Performance is a separate measurement; unsupported compilation is an explicit unsupported result, never substituted with `HSA_OVERRIDE_GFX_VERSION`.

## S17-EXP-004 — Vulkan/RADV capability and heap capture

```bash
date -u +%FT%TZ
vulkaninfo --summary
vulkaninfo --json > vulkaninfo.json
```

Record loader, ICD, Mesa, kernel, device name/UUID, API version, subgroup sizes/operations, storage-buffer limits, memory heaps/types/budgets, external-memory support, and cooperative-matrix properties/extensions. Pass evidence: the intended Radeon 8060S is selected and a project correctness fixture runs without validation errors. Vulkan 1.4 alone does not pass a cooperative-matrix requirement.

## S17-EXP-005 — Shared-memory and sustained package behavior

Prerequisites: section 73 methodology and section 22 sensor authority. Root: no for ordinary sensors; any power-policy change is out of scope.

Run a matched, reversible matrix on each host: idle baseline; CPU-only memory traffic; GPU-only read/write/copy kernels; simultaneous CPU+GPU traffic; representative inference prefill/decode. Record actual clocks, package/GPU power if available, temperatures, throttling flags, wall time, bytes transferred, and workload hashes. Include warm-up and thermal steady-state intervals.

Pass evidence: raw time series plus derived bandwidth/throughput, with units and environment. **[MEASURED]** labels may be added only after raw artifacts and metadata are linked.

## Machine experiment register

| ID | Owner/dependency | Resolves |
|---|---|---|
| S17-EXP-001 | section 17/18 | CPU cache-sharing and complex topology |
| S17-EXP-002 | section 17/19/23/24 | `gfx1151`, CU/cache/pool identity and carveout |
| S17-EXP-003 | section 17/23/37/74 | native compiler/library datatype and WMMA correctness |
| S17-EXP-004 | section 17/25/74 | RADV device, heaps, subgroup and matrix exposure |
| S17-EXP-005 | section 17/22/73/74 | effective memory bandwidth and sustained power/clock behavior |
