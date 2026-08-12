# 08 — Fabric and Transport

## Category manifest

- **Purpose:** Define dual-USB4 transport requirements, protocol behavior, and validation.
- **Authoritative files:** This manifest and the seven linked section artifact sets.
- **Current owner:** Fabric workers own measurements. Documentation workers own routing.
- **Status:** Populated. Integrated protocol and machine validation remain pending.
- **Last verified date:** 2026-07-29 for routing. Technical review remains dated 2026-07-17.
- **Source commits:** Linux `fce2dfa773ced15f27dd27cd0b482a7473cdcf2a` and `8cdeaa50eae8dad34885515f62559ee83e7e8dda`.
- **Related decisions:** [Decision map](../decision-map.md) routes the accepted distributed execution decisions.
- **Related evidence:** [Evidence map](../evidence-map.md) and the [Section 55 experiment plan](55_Fabric_Microbenchmark_Plan_and_USB4_Kernel_Patch_Decision/README.md).
- **Open work:** Select and qualify an exact deployment kernel and transport mode.
- **Next safe action:** Re-diff the driver and testing interface before deployment qualification.

Defines the dual-USB4 communication layer and the experiments needed before specializing it.

Research status: populated; integrated protocol and machine validation remain pending as of 2026-07-17.

## Evidence and kernel authority

Sections 49–52 cite Linux commit `fce2dfa773ced15f27dd27cd0b482a7473cdcf2a`.
Sections 53–55 cite Linux commit `8cdeaa50eae8dad34885515f62559ee83e7e8dda` at Linux v7.2-rc2.
The commits are inspected **source snapshots**.
The commits do not approve a deployment kernel.

The 2026-07-17 review found `drivers/thunderbolt/stream.c` byte-identical at both commits.
Byte identity does not establish packaging or target-hardware compatibility.
Byte identity does not establish the behavior of a future stable kernel.

When the project selects a deployment candidate, re-diff the driver and testing application binary interface.
Record the exact package, configuration, and module hashes.
Run the Section 50 and Section 55 capability and crossover gates before promotion.

Section 53 is the proposed wire-contract authority.
The version 1 recovery rule applies globally.
Loss or reconnection of either rail creates a barrier across all rails.
The failure ends the epoch.
Retry may occur only as one complete idempotent upper-layer operation.
The retry must use a newly authenticated epoch.
Sections 49, 52, and 55 defer to the Section 53 rule.

Remote direct memory access (RDMA) appears in the Section 51 canonical title.

- [49 — Fabric Requirements and Transport Abstraction](49_Fabric_Requirements_and_Transport_Abstraction/README.md)
- [50 — USB4STREAM and thunderbolt-net Implementation Options](50_USB4STREAM_and_thunderbolt_net_Implementation_Options/README.md)
- [51 — Existing ggml RPC and ROCmFPX RDMA Transport Audit](51_Existing_ggml_RPC_and_ROCmFPX_RDMA_Transport_Audit/README.md)
- [52 — Dual-Link Multipath: Striping, Alternation, Hedging, and Failover](52_Dual_Link_Multipath_Striping_Alternation_Hedging_and_Failover/README.md)
- [53 — Message Framing, Credits, Flow Control, Integrity, and Security](53_Message_Framing_Credits_Flow_Control_Integrity_and_Security/README.md)
- [54 — GPU-Visible Buffers, Coherence, Copies, and Zero-Copy Options](54_GPU_Visible_Buffers_Coherence_Copies_and_Zero_Copy_Options/README.md)
- [55 — Fabric Microbenchmark Plan and USB4 Kernel-Patch Decision](55_Fabric_Microbenchmark_Plan_and_USB4_Kernel_Patch_Decision/README.md)
