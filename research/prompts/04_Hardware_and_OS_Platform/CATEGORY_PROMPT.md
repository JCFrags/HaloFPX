# Category Research Agent Prompt — 04: Hardware and OS Platform

The project is a custom local LLM inference stack for two matched AMD Strix Halo systems connected by two tuned, low-latency USB4 host-to-host links. The intended source base is a fork of `charlie12345/ROCmFPX`, selectively incorporating the persistent SSD-backed KV-cache and agent-serving ideas from `fewtarius/CachyLLama` and `fewtarius/llama-ai`, while tracking `ggml-org/llama.cpp`. The product must choose among replication, remote speculative decoding, two-way tensor parallelism, pipeline parallelism, and MoE-aware hybrid execution, with a rank-local persistent cache and a specialized dual-link transport. The wiki will be consumed by local Codex and human engineers before and during implementation.

Build the complete wiki category **04: Hardware and OS Platform**. Research every numbered section below as a separate self-contained folder:

- `17_AMD_Strix_Halo_SoC_and_gfx1151_Architecture/` — 17: AMD Strix Halo SoC and gfx1151 Architecture
- `18_Exact_Machine_BOM_BIOS_Firmware_Cabling_and_Revisions/` — 18: Exact Machine BOM, BIOS, Firmware, Cabling, and Revisions
- `19_Unified_Memory_GTT_GPUVM_IOMMU_and_Allocation_Limits/` — 19: Unified Memory, GTT, GPUVM, IOMMU, and Allocation Limits
- `20_USB4_Physical_Topology_and_Dual_Port_Independence/` — 20: USB4 Physical Topology and Dual-Port Independence
- `21_NVMe_and_Storage_Topology_Performance_and_Endurance/` — 21: NVMe and Storage Topology, Performance, and Endurance
- `22_Power_Thermals_Cooling_and_Sustained_Clocks/` — 22: Power, Thermals, Cooling, and Sustained Clocks
- `23_Linux_Kernel_amdgpu_Firmware_ROCm_and_Mesa_Compatibility_Matrix/` — 23: Linux, Kernel, amdgpu, Firmware, ROCm, and Mesa Compatibility Matrix

Apply `../OUTPUT_STANDARD.md` to every section. Use current primary sources, exact commits/versions/dates, and explicit claim labels. Separate Internet research from required on-machine investigation. Do not invent measurements or silently resolve conflicting evidence.

Return one downloadable folder named `04_Hardware_and_OS_Platform/` containing the numbered section folders at the exact paths listed above, plus a category `README.md` that summarizes the category, identifies authoritative pages, links all sections, and highlights cross-category dependencies.
