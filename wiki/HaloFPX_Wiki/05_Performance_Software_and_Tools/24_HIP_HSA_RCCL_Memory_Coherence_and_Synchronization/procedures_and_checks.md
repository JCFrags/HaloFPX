---
section_id: "24"
title: "HIP, HSA, and RCCL procedures and checks"
status: "needs-machine-validation"
last_verified: "2026-07-16"
applies_to:
  repositories: ["ROCm 7.2.3 probes; HaloFPX experiment implementation pending"]
  software_versions: ["ROCm 7.2.3 research baseline"]
  hardware_revisions: ["run independently on both gfx1151 hosts"]
related_sections: ["18", "19", "23", "27", "74", "75"]
---

# Procedures and checks

All checks are non-destructive. Run on both machines, store raw output under the project-wide `experiments/` authority, and link it back here. Do not label a result `[MEASURED]` until the command, source, binary hash, environment, and raw output are preserved.

## EX24-01: software and agent inventory

**Purpose:** prove the exact environment. Root is not required for the read-only commands; `dmesg` may require elevated access.

```bash
date --iso-8601=seconds
uname -a
cat /etc/os-release
hipconfig --full
rocminfo
rocm-smi --showproductname --showuniqueid --showmeminfo all
dpkg-query -W 'rocm*' 'hip*' 'hsa*' 'rccl*' 2>/dev/null || true
lspci -nnk
```

Record BIOS/firmware revision through the section 18 procedure and kernel/amdgpu firmware through section 23. Acceptance: both hosts have complete, independently identified records; differences are explicit.

## EX24-02: HIP capability probe

Compile a small HIP program that emits JSON for `hipGetDeviceProperties` and `hipDeviceGetAttribute`, including:

- `gcnArchName`, `integrated`, `unifiedAddressing`, `canMapHostMemory`;
- `hipDeviceAttributeFineGrainSupport`;
- `hipDeviceAttributeHostNativeAtomicSupported`;
- `hipDeviceAttributeCanUseStreamWaitValue`;
- managed/pageable/concurrent-managed access;
- async engine count, stream priority, memory-pool and host-register support.

Compile with the exact installed `hipcc` and `--offload-arch=gfx1151`. Acceptance: JSON schema and binary SHA-256 are preserved for each host.

## EX24-03: HSA pool and pointer classification

Enumerate CPU and GPU agents and all `hsa_amd_memory_pool_get_info` fields relevant to location, global flags, accessibility, allocation granule, maximum size, and agent access. Allocate each HIP class with explicit flags and record `hipPointerGetAttributes` plus HSA pool properties where observable.

Test at least:

- `hipMalloc`;
- `hipExtMallocWithFlags(..., hipDeviceMallocFinegrained)`;
- `hipHostMallocMapped | hipHostMallocCoherent`;
- `hipHostMallocNonCoherent`;
- `hipMallocManaged` with and without coarse-grain advice.

Acceptance: no conclusion is drawn from API name alone; returned properties and failures are retained.

## EX24-04: CPU/GPU coherence litmus tests

For each supported allocation mode, run bounded producer/consumer tests in both directions using payload, checksum, and monotonically increasing sequence fields. Compare:

1. no explicit fence (negative control; never a production pattern);
2. kernel `__threadfence_system` plus system-scope atomic publication;
3. default event + `hipEventSynchronize`;
4. event created with `hipEventReleaseToSystem` plus synchronization;
5. event created with `hipEventDisableSystemFence` (negative control);
6. explicit async copy followed by an explicit system-release event.

Acceptance: zero mismatches across a declared iteration count is evidence only for that environment/run; preserve failures, timeouts, compiler flags, and CPU atomic operations. A hang must terminate by watchdog and be recorded.

## EX24-05: stream/event/queue mapping

Run one, two, four, and eight non-default streams with small kernels and copies. Use explicit fork/join events. Trace without counters:

```bash
rocprofv3 --hip-trace --hsa-trace --kernel-trace \
  --memory-copy-trace --memory-allocation-trace \
  --output-format csv pftrace -- ./ex24_stream_probe
```

Acceptance: report HIP stream IDs, HSA/ROCm queue IDs, overlap, launch gaps, and whether results differ between hosts. Do not infer physical concurrency from enqueue timing alone.

## EX24-06: graph-capture compatibility and overhead

Capture the exact repeated compute/copy sequence, list every capture-rejected API and error, then compare uncaptured streams, captured graph replay, and graph update. Keep transport I/O outside capture. Acceptance: correctness matches the stream oracle; report CPU launch time and end-to-end time separately after warm-up.

## EX24-07: local peer/API boundary

Record `hipGetDeviceCount` and the full `hipDeviceCanAccessPeer` matrix. On the expected one-GPU-per-host topology, explicitly record that the remote host is absent from the local device list. Do not attempt to force peer enablement after a `0` capability result.

## EX24-08: GPU-produced transport-buffer comparison

Compare paths A-D from [design implications](design_implications.md) over powers-of-two payloads covering actual activation/collective sizes. Validate bytes and generation metadata before timing. Report allocation/registration cost, warm steady state, CPU time, GPU time, latency distribution, throughput, and failure behavior.

## EX24-09: RCCL two-rank baseline

Pin exact RCCL build/commit and log environment. Test one collective at a time with `NCCL_DEBUG=INFO`, explicit `NCCL_SOCKET_IFNAME`, timeouts, and matched message sets. Run each physical link separately before any bonding/multipath mode. Acceptance: rank mapping, interface selection, topology log, correctness, timeout, restart, and single-node fallback are recorded.

## EX24-10: profiler perturbation check

Repeat a fixed workload unprofiled, with runtime tracing, and with selected counters. Acceptance: quantify trace/counter overhead and never compare a profiled candidate against an unprofiled baseline.

## Decision gates

| Decision | Required experiments |
|---|---|
| Enable mapped fine-grained publication | EX24-02, EX24-03, EX24-04, EX24-08 |
| Use graph replay in steady state | EX24-05, EX24-06, EX24-10 |
| Use RCCL sockets or plugin | EX24-08, EX24-09 plus section 75 |
| Use system-scope atomic ready words | EX24-02 and zero-error EX24-04 on both hosts |
