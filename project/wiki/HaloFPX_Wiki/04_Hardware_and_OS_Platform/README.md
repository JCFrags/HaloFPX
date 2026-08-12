# 04 — Hardware and OS Platform

## Category manifest

- **Purpose:** Record the two-node hardware and operating-system constraints.
- **Authoritative files:** This manifest, the seven linked section artifact sets, and retained machine evidence.
- **Current owner:** Machine operators own observations. Documentation workers own routing.
- **Status:** Structurally complete. All section metadata passes the Wiki validator. Target-machine validation remains open.
- **Last verified date:** 2026-07-29 for routing. Section claims retain their own dates.
- **Source commits:** Section-specific kernel and software commits remain in each `section.yaml` and source ledger.
- **Related decisions:** [Decision map](../decision-map.md) and current [Project Lead decisions](../../../project-management/lead/DECISIONS.md).
- **Related evidence:** [Evidence map](../evidence-map.md) and [experiments](../../../experiments/).
- **Open work:** Revalidate the exact machine bill of materials, firmware, software tuple, and link topology when the environment changes.
- **Next safe action:** Record the host, date, command, binary, and exact result for each machine observation.

Documents the two physical systems and every platform constraint that affects performance or correctness.

Artifact state: 7/7 required section artifact sets are present. This is structural completeness, not research acceptance or machine validation.

- [17 — AMD Strix Halo SoC and gfx1151 Architecture](17_AMD_Strix_Halo_SoC_and_gfx1151_Architecture/README.md)
- [18 — Exact Machine BOM, BIOS, Firmware, Cabling, and Revisions](18_Exact_Machine_BOM_BIOS_Firmware_Cabling_and_Revisions/README.md)
- [19 — Unified Memory, GTT, GPUVM, IOMMU, and Allocation Limits](19_Unified_Memory_GTT_GPUVM_IOMMU_and_Allocation_Limits/README.md)
- [20 — USB4 Physical Topology and Dual-Port Independence](20_USB4_Physical_Topology_and_Dual_Port_Independence/README.md)
- [21 — NVMe and Storage Topology, Performance, and Endurance](21_NVMe_and_Storage_Topology_Performance_and_Endurance/README.md)
- [22 — Power, Thermals, Cooling, and Sustained Clocks](22_Power_Thermals_Cooling_and_Sustained_Clocks/README.md)
- [23 — Linux, Kernel, amdgpu, Firmware, ROCm, and Mesa Compatibility Matrix](23_Linux_Kernel_amdgpu_Firmware_ROCm_and_Mesa_Compatibility_Matrix/README.md)
