---
section_id: "24"
title: "HIP, HSA, and RCCL open questions"
status: "needs-machine-validation"
last_verified: "2026-07-16"
applies_to:
  repositories: ["HaloFPX design; implementation commit not selected"]
  software_versions: ["ROCm 7.2.3 research baseline"]
  hardware_revisions: ["two exact gfx1151 machines not yet inventoried"]
related_sections: ["18", "19", "23", "42", "54", "75"]
---

# Open questions

| ID | Question | Why it matters | Resolution evidence |
|---|---|---|---|
| OQ24-01 | What exact BIOS, kernel, firmware, and ROCm 7.2.x set runs on each host? | All lower-level behavior is version- and machine-scoped. | EX24-01 plus sections 18/23. |
| OQ24-02 | Which HSA pools and HIP capability attributes does each gfx1151 agent report? | Determines legal fine-grained, atomic, mapping, and allocation paths. | EX24-02/03 raw JSON. |
| OQ24-03 | What is the observed coherence of every explicit and default host allocation mode? | Official HIP documentation is inconsistent about the default. | EX24-03/04; upstream clarification if needed. |
| OQ24-04 | Are system-scope CPU/GPU atomics and explicit `hipEventReleaseToSystem` publication correct and performant? | Required for GPU-produced buffers; official event text is internally inconsistent. | EX24-04/08, both hosts. |
| OQ24-05 | Does mapped fine-grained host memory beat an explicit device-to-host copy at real payload sizes? | Unified physical memory does not guarantee a faster mapped path. | EX24-08 distributions, not peak-only numbers. |
| OQ24-06 | How do HIP streams map to queues on the pinned runtime, and what overlap is stable? | Affects command-ring count, backpressure, and graph structure. | EX24-05 traces. |
| OQ24-07 | Which llama.cpp/ROCmFPX operations reject capture or graph update? | Determines whether steady-state graph replay is viable. | EX24-06 on exact source commit. |
| OQ24-08 | Can RCCL socket transport use both USB4 paths predictably, and at what crossover does it help? | Determines baseline collective engine and topology policy. | EX24-09 plus sections 52/75. |
| OQ24-09 | Is a custom `librccl-net` plugin technically and operationally better than the project transport? | Plugin ABI work is not justified without end-to-end gain. | Prototype only after EX24-08/09 and section 75. |
| OQ24-10 | Which buffer/export/registration mechanisms are available for gfx1151 and the selected USB4 transport? | Controls whether direct device-pointer or DMA-BUF paths are possible. | Driver/API probe, registration test, and source audit in section 54. |
| OQ24-11 | What is the failure contract for rank loss during a collective or buffer transfer? | Partial output must never be consumed. | Fault injection with communicator restart and single-node fallback. |
| OQ24-12 | How much do rocprofv3 tracing and counters perturb short-message latency? | Prevents profiler-induced design conclusions. | EX24-10 and section 27. |

## Internet follow-up

- **[OPEN]** Track ROCm/HIP release notes for gfx1151 coherence, graph-capture, AQL queue, and host-native-atomic fixes after tag `rocm-7.2.3`.
- **[OPEN]** Audit the exact RCCL 2.27.7 network-plugin ABI and socket implementation at the commit selected by the build lockfile, including pointer-support and DMA-BUF branches.
- **[OPEN]** Locate an authoritative AMD resolution for the current `hipHostMallocDefault` coherence documentation inconsistency; retain the machine probe regardless.
- **[OPEN]** Audit the selected ROCmFPX/llama.cpp commits for `hipEventDisableSystemFence`, implicit null-stream use, mapped host buffers, HIP graph capture, and RCCL integration.

## Review triggers

Re-review this section when the ROCm minor release, kernel/amdgpu firmware, BIOS, RCCL commit, transport backend, or buffer protocol changes, or when either machine reports a different HSA pool/capability inventory.
