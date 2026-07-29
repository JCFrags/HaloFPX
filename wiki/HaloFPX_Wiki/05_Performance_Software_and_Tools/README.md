# 05 — Performance Software and Tools

## Category manifest

- **Purpose:** Define runtime, toolchain, profiling, and host-tuning guidance.
- **Authoritative files:** This manifest and the five linked section artifact sets.
- **Current owner:** Performance workers own measurements. Documentation workers own routing.
- **Status:** Source-backed draft complete. Machine qualification and policy selection remain open.
- **Last verified date:** 2026-07-29 for routing. Section claims retain their own dates.
- **Source commits:** Exact tool and runtime commits remain in each section source ledger.
- **Related decisions:** [Decision map](../decision-map.md) and [Project Lead objectives](../../../project-management/lead/OBJECTIVES.md).
- **Related evidence:** [Evidence map](../evidence-map.md) and [Verification and Performance](../11_Verification_and_Performance/README.md).
- **Open work:** Qualify the exact target-machine software tuple and measurement controls.
- **Next safe action:** Freeze exact versions before a tool or tuning comparison.

Defines the low-level runtimes, build tools, profilers, and host tuning needed to measure and optimize the platform.

Research status: source-backed draft complete; target-machine qualification and policy selection remain open.

- [24 — HIP, HSA, RCCL, Memory Coherence, and Synchronization](24_HIP_HSA_RCCL_Memory_Coherence_and_Synchronization/README.md)
- [25 — Vulkan, RADV, Host-Visible Memory, and Synchronization](25_Vulkan_RADV_Host_Visible_Memory_and_Synchronization/README.md)
- [26 — Compiler, CMake, Linker, and Reproducible Toolchain](26_Compiler_CMake_Linker_and_Reproducible_Toolchain/README.md)
- [27 — Profiling, Tracing, Debugging, and Hardware-Counter Collection](27_Profiling_Tracing_Debugging_and_Hardware_Counter_Collection/README.md)
- [28 — Host System Tuning: CPU, IRQ, Scheduler, Cgroups, and Filesystems](28_Host_System_Tuning_CPU_IRQ_Scheduler_Cgroups_and_Filesystems/README.md)

All five sections remain `needs-machine-validation`. Their recommendations are candidate controls, not an approved software tuple, tuning profile, or performance result.
