---
section_id: "24"
title: "HIP, HSA, and RCCL source ledger"
status: "draft"
last_verified: "2026-07-16"
applies_to:
  repositories:
    - "ROCm/HIP@bc9af25177f96c0fea93198b89cf4c3cf08f3ea3"
    - "ROCm/ROCR-Runtime@e5498ba92dad7099d2027bd22bd7295ca1caf833"
    - "ROCm/rccl@96a25b5fd6f73fba58c7d83eb57cf19a50230aa4"
  software_versions: ["ROCm 7.2.3", "HIP 7.2.53211 documentation", "RCCL 2.27.7"]
  hardware_revisions: ["gfx1151 family only; exact hosts unverified"]
related_sections: ["02", "19", "23", "27", "54"]
---

# Source ledger

All sources are public primary vendor documentation or exact upstream source/tag references. Access date is 2026-07-16.

| ID | Source and stable locator | Revision/date | Claims supported | Limitations/conflicts |
|---|---|---|---|---|
| S24-001 | AMD, [Strix Halo system optimization](https://rocm.docs.amd.com/en/docs-7.2.0/how-to/system-optimization/strixhalo.html) | ROCm 7.2 docs; page dated 2026-02-20 | gfx1151 GPUVM and unified-memory platform description | Guidance, not either machine's inventory or benchmark. |
| S24-002 | AMD, [Linux support matrices by ROCm version](https://rocm.docs.amd.com/projects/radeon-ryzen/en/docs-7.2/docs/compatibility/compatibilityryz/native_linux/native_linux_compatibility.html) | ROCm 7.2 | gfx1151/Ryzen AI Max support | Support is conditional on listed OS/software; not all APIs or data types validated. |
| S24-003 | AMD HIP, [Memory management](https://rocm.docs.amd.com/projects/HIP/en/docs-7.2.1/how-to/hip_runtime_api/memory_management.html) and [coherence control](https://rocm.docs.amd.com/projects/HIP/en/docs-7.2.1/how-to/hip_runtime_api/memory_management/coherence_control.html) | HIP 7.2.53211 docs | allocation classes, flags, fine/coarse modes, synchronization visibility | Internal default-`hipHostMalloc` and event-scope inconsistencies are retained as OQ24-03/OQ24-04. |
| S24-004 | AMD HIP, [Host memory](https://rocm.docs.amd.com/projects/HIP/en/docs-7.2.0/how-to/hip_runtime_api/memory_management/host_memory.html) | HIP 7.2.53210 docs | mapped/coherent/noncoherent pinned host allocations, NUMA note | Describes API semantics, not gfx1151 performance. |
| S24-005 | AMD HIP, [HIP C++ language extensions](https://rocm.docs.amd.com/projects/HIP/en/docs-7.2.0/reference/kernel_language.html) | HIP 7.2.0 docs | fence scopes and system-scope atomic variants | Hardware support is conditional and must be queried/tested. |
| S24-006 | ROCm/HIP, [`hip_runtime_api.h`](https://github.com/ROCm/HIP/blob/bc9af25177f96c0fea93198b89cf4c3cf08f3ea3/include/hip/hip_runtime_api.h) | tag `rocm-7.2.3`, commit `bc9af25177f96c0fea93198b89cf4c3cf08f3ea3` | capability fields/attributes, event and allocation flags, API surface | Header declarations do not prove runtime support on a machine. |
| S24-007 | AMD ROCR, [HSA runtime API](https://rocm.docs.amd.com/projects/ROCR-Runtime/en/develop/api-reference/api.html) and [ROCR source](https://github.com/ROCm/ROCR-Runtime/tree/e5498ba92dad7099d2027bd22bd7295ca1caf833) | ROCR docs 1.21.0 observed; `rocm-7.2.3` commit `e5498ba92dad7099d2027bd22bd7295ca1caf833` | memory-pool flags, queue/packet fence scopes, signals, allocation properties | `develop` docs can move; source commit is the implementation pin. Machine pool exposure remains open. |
| S24-008 | AMD HIP, [Event management](https://rocm.docs.amd.com/projects/HIP/en/docs-7.2.1/reference/hip_runtime_api/modules/event_management.html) and [stream management](https://rocm.docs.amd.com/projects/HIP/en/docs-7.2.1/reference/hip_runtime_api/modules/stream_management.html) | HIP 7.2.53211 docs | event flags, system-fence disable semantics, stream wait/synchronize behavior | API reference does not quantify latency or scheduling. |
| S24-009 | AMD HIP, [HIP Graph API tutorial](https://rocm.docs.amd.com/projects/HIP/en/docs-7.2.0/tutorial/graph_api.html) | HIP 7.2.53210 docs | stream ordering, cross-stream dependencies, graph tradeoffs, tracing example | Tutorial results are illustrative and not project measurements. |
| S24-010 | AMD HIP, [HIP graphs](https://rocm.docs.amd.com/projects/HIP/en/docs-7.2.1/how-to/hip_runtime_api/hipgraph.html) | HIP 7.2.53211 docs | capture semantics and node types | Exact application API compatibility must be tested. |
| S24-011 | AMD, [ROCm consolidated changelog](https://rocm.docs.amd.com/en/latest/release/changelog.html) | HIP 7.2.1 resolved issues; accessed 2026-07-16 | capture/event fixes and AQL batch-dispatch doorbell fix | Changelog is newer/moving; baseline is pinned to ROCm 7.2.3. |
| S24-012 | AMD ROCprofiler-SDK, [Using rocprofv3](https://rocm.docs.amd.com/projects/rocprofiler-sdk/en/develop/how-to/using-rocprofv3.html) and [CLI options](https://rocm.docs.amd.com/projects/rocprofiler-sdk/en/develop/quick-reference/rocprofv3-cli-options.html) | docs 1.3.2 observed | HIP/HSA/kernel/copy/allocation/KFD/RCCL tracing and record fields | Tool version must match installed ROCm; profiling perturbs execution. |
| S24-013 | AMD HIP, [Peer-to-peer device memory access](https://rocm.docs.amd.com/projects/HIP/en/docs-7.0.2/reference/hip_runtime_api/modules/peer_to_peer_device_memory_access.html) | HIP 7.0.51831 docs | `hipDeviceCanAccessPeer` semantics | Older API-doc build; verify against pinned 7.2.3 header/runtime. Does not cover network peers. |
| S24-014 | AMD RCCL, [Using the NCCL Net plugin API](https://rocm.docs.amd.com/projects/rccl/en/docs-7.2.0/how-to/using-nccl.html) | RCCL 2.27.7 in ROCm 7.2 documentation | plugin loading, naming, versioned ABI, registration and flush contract | Documentation presents API v6; exact 7.2.3 source pin is S24-015. |
| S24-015 | ROCm/RCCL, [network-plugin header](https://github.com/ROCm/rccl/blob/96a25b5fd6f73fba58c7d83eb57cf19a50230aa4/ext-net/example/nccl/net.h) | tag `rocm-7.2.3`, commit `96a25b5fd6f73fba58c7d83eb57cf19a50230aa4` | host/device/DMA-BUF pointer capability bits and ABI versions | Capability bits do not prove a given plugin or gfx1151 path works. |
| S24-016 | AMD RCCL, [Environment variables](https://rocm.docs.amd.com/projects/rccl/en/docs-7.2.0/api-reference/env-variables.html) | ROCm 7.2 / RCCL documentation | socket interface, logging, algorithm/protocol controls | Environment variables can change; preserve runtime logs and configuration. |
| S24-017 | ROCm/ROCm, [ROCm 7.2.3 release](https://github.com/ROCm/ROCm/releases/tag/rocm-7.2.3) | release 2026-05-04; commit `14f8138863403a26e0caef6671cfab9b09aa636e` | current research baseline identity | Release support does not substitute for per-component pins or machine validation. |

## Source gaps

- No preserved machine output exists yet for either gfx1151 host.
- No selected HaloFPX, ROCmFPX, llama.cpp, kernel, or firmware commit was supplied to this section.
- No primary source establishes a ready-made RCCL plugin for the dual USB4 host-to-host design.
- No primary source resolves the current default-host-allocation coherence documentation inconsistency.
