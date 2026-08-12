# HaloFPX LLM Wiki Research Prompt Package

The independently reviewed follow-up backlog is [`2026-07-17__pre-fork-further-internet-research-prompts.md`](2026-07-17__pre-fork-further-internet-research-prompts.md). None of those prompts blocks the accepted local/read-only L00A preparation lane.

This package defines a modular, evidence-driven LLM wiki for the proposed dual–AMD Strix Halo inference project. It contains **12 category prompts** and **86 standalone section prompts**. Each assignment asks a research agent to return a downloadable folder at a stable path so the results can be merged into one local wiki for Codex and human engineers.

## Intended source context

- `https://github.com/charlie12345/ROCmFPX`
- `https://github.com/fewtarius/CachyLLama`
- `https://github.com/fewtarius/llama-ai`
- `https://github.com/ggml-org/llama.cpp`

## How to use

1. For one section, give the research agent that section folder, especially `PROMPT.md` and `OUTPUT_STANDARD.md`.
2. For a whole category, give the agent the category's `CATEGORY_PROMPT.md` plus the root `OUTPUT_STANDARD.md`.
3. Require the agent to return the exact target folder path named in the prompt.
4. Merge returned research into a separate `HaloFPX_Wiki/` tree, not into this prompt package.
5. Preserve existing verified material before replacing it; archive superseded pages and record the change.
6. Run `python tools/validate_wiki.py /path/to/HaloFPX_Wiki` to identify missing required files.
7. Copy `AGENTS.md.template` to the project root as `AGENTS.md` and adapt it for the local Codex workflow.

## Granularity choices

- **Section run:** best for parallel research, narrow context, and reviewable outputs.
- **Category run:** best when one agent needs to resolve cross-links among closely related sections.
- **Orchestrated run:** use `MASTER_ORCHESTRATOR_PROMPT.md` only with an agent capable of producing many folders without losing source discipline.

## Category index

| Category | Sections | Purpose |
|---|---:|---|
| [01 — Wiki Governance](01_Wiki_Governance/README.md) | 5 | Defines how the wiki is organized, sourced, versioned, and consumed by Codex and human engineers. |
| [02 — Project Definition](02_Project_Definition/README.md) | 5 | Establishes why the product exists, what it must do, and how success will be judged. |
| [03 — Repository and Engineering](03_Repository_and_Engineering/README.md) | 6 | Maps the source lineage, code structure, integration approach, and engineering controls. |
| [04 — Hardware and OS Platform](04_Hardware_and_OS_Platform/README.md) | 7 | Documents the two physical systems and every platform constraint that affects performance or correctness. |
| [05 — Performance Software and Tools](05_Performance_Software_and_Tools/README.md) | 5 | Defines the low-level runtimes, build tools, profilers, and host tuning needed to measure and optimize the platform. |
| [06 — Models, Quantization, and Inference](06_Models_Quantization_and_Inference/README.md) | 9 | Captures the supported model families, numerical formats, engine behavior, and kernel optimization surface. |
| [07 — Distributed Runtime](07_Distributed_Runtime/README.md) | 11 | Defines how two Strix Halo machines cooperate and when each parallel execution mode is used. |
| [08 — Fabric and Transport](08_Fabric_and_Transport/README.md) | 7 | Defines the dual-USB4 communication layer and the experiments needed before specializing it. |
| [09 — HaloKV Persistent Cache](09_HaloKV_Persistent_Cache/README.md) | 10 | Defines the distributed SSD-backed prefix and inference-state cache. |
| [10 — Product, Server, and Operations](10_Product_Server_and_Operations/README.md) | 7 | Defines the user-facing service and how it is installed, secured, observed, and maintained. |
| [11 — Verification and Performance](11_Verification_and_Performance/README.md) | 9 | Defines the measurements and gates that determine whether the system is correct, faster, and releasable. |
| [12 — Project Execution and Governance](12_Project_Execution_and_Governance/README.md) | 5 | Turns the technical design into an actionable, traceable implementation program. |

## Complete section index

### 01 — Wiki Governance

- [01 — Wiki Architecture, Navigation, and Root Manifest](01_Wiki_Governance/01_Wiki_Architecture_Navigation_and_Root_Manifest/PROMPT.md)
- [02 — Evidence, Citation, and Source Policy](01_Wiki_Governance/02_Evidence_Citation_and_Source_Policy/PROMPT.md)
- [03 — Glossary, Naming, and Stable Identifiers](01_Wiki_Governance/03_Glossary_Naming_and_Stable_Identifiers/PROMPT.md)
- [04 — Assumption, Open-Question, and Decision Ledgers](01_Wiki_Governance/04_Assumption_Open_Question_and_Decision_Ledgers/PROMPT.md)
- [05 — Research Data and Benchmark Artifact Conventions](01_Wiki_Governance/05_Research_Data_and_Benchmark_Artifact_Conventions/PROMPT.md)

### 02 — Project Definition

- [06 — Project Charter, Vision, and Intended Outcomes](02_Project_Definition/06_Project_Charter_Vision_and_Intended_Outcomes/PROMPT.md)
- [07 — Users, Workloads, Personas, and Use Cases](02_Project_Definition/07_Users_Workloads_Personas_and_Use_Cases/PROMPT.md)
- [08 — Scope, Non-Goals, Boundaries, and External Dependencies](02_Project_Definition/08_Scope_Non_Goals_Boundaries_and_External_Dependencies/PROMPT.md)
- [09 — Functional Requirements, SLOs, and Acceptance Criteria](02_Project_Definition/09_Functional_Requirements_SLOs_and_Acceptance_Criteria/PROMPT.md)
- [10 — Architecture Principles and Tradeoff Framework](02_Project_Definition/10_Architecture_Principles_and_Tradeoff_Framework/PROMPT.md)

### 03 — Repository and Engineering

- [11 — Repository Lineage, Branches, Commits, and Frozen Baselines](03_Repository_and_Engineering/11_Repository_Lineage_Branches_Commits_and_Frozen_Baselines/PROMPT.md)
- [12 — Codebase Architecture and Module Map](03_Repository_and_Engineering/12_Codebase_Architecture_and_Module_Map/PROMPT.md)
- [13 — ROCmFPX Feature, Kernel, Format, and Patch Inventory](03_Repository_and_Engineering/13_ROCmFPX_Feature_Kernel_Format_and_Patch_Inventory/PROMPT.md)
- [14 — llama-ai and CachyLLama Feature and Patch Inventory](03_Repository_and_Engineering/14_llama_ai_and_CachyLLama_Feature_and_Patch_Inventory/PROMPT.md)
- [15 — Integration Patch Stack and Upstream Synchronization Strategy](03_Repository_and_Engineering/15_Integration_Patch_Stack_and_Upstream_Synchronization_Strategy/PROMPT.md)
- [16 — Build, Dependencies, Licensing, CI, and AI-Agent Workflow](03_Repository_and_Engineering/16_Build_Dependencies_Licensing_CI_and_AI_Agent_Workflow/PROMPT.md)

### 04 — Hardware and OS Platform

- [17 — AMD Strix Halo SoC and gfx1151 Architecture](04_Hardware_and_OS_Platform/17_AMD_Strix_Halo_SoC_and_gfx1151_Architecture/PROMPT.md)
- [18 — Exact Machine BOM, BIOS, Firmware, Cabling, and Revisions](04_Hardware_and_OS_Platform/18_Exact_Machine_BOM_BIOS_Firmware_Cabling_and_Revisions/PROMPT.md)
- [19 — Unified Memory, GTT, GPUVM, IOMMU, and Allocation Limits](04_Hardware_and_OS_Platform/19_Unified_Memory_GTT_GPUVM_IOMMU_and_Allocation_Limits/PROMPT.md)
- [20 — USB4 Physical Topology and Dual-Port Independence](04_Hardware_and_OS_Platform/20_USB4_Physical_Topology_and_Dual_Port_Independence/PROMPT.md)
- [21 — NVMe and Storage Topology, Performance, and Endurance](04_Hardware_and_OS_Platform/21_NVMe_and_Storage_Topology_Performance_and_Endurance/PROMPT.md)
- [22 — Power, Thermals, Cooling, and Sustained Clocks](04_Hardware_and_OS_Platform/22_Power_Thermals_Cooling_and_Sustained_Clocks/PROMPT.md)
- [23 — Linux, Kernel, amdgpu, Firmware, ROCm, and Mesa Compatibility Matrix](04_Hardware_and_OS_Platform/23_Linux_Kernel_amdgpu_Firmware_ROCm_and_Mesa_Compatibility_Matrix/PROMPT.md)

### 05 — Performance Software and Tools

- [24 — HIP, HSA, RCCL, Memory Coherence, and Synchronization](05_Performance_Software_and_Tools/24_HIP_HSA_RCCL_Memory_Coherence_and_Synchronization/PROMPT.md)
- [25 — Vulkan, RADV, Host-Visible Memory, and Synchronization](05_Performance_Software_and_Tools/25_Vulkan_RADV_Host_Visible_Memory_and_Synchronization/PROMPT.md)
- [26 — Compiler, CMake, Linker, and Reproducible Toolchain](05_Performance_Software_and_Tools/26_Compiler_CMake_Linker_and_Reproducible_Toolchain/PROMPT.md)
- [27 — Profiling, Tracing, Debugging, and Hardware-Counter Collection](05_Performance_Software_and_Tools/27_Profiling_Tracing_Debugging_and_Hardware_Counter_Collection/PROMPT.md)
- [28 — Host System Tuning: CPU, IRQ, Scheduler, Cgroups, and Filesystems](05_Performance_Software_and_Tools/28_Host_System_Tuning_CPU_IRQ_Scheduler_Cgroups_and_Filesystems/PROMPT.md)

### 06 — Models, Quantization, and Inference

- [29 — Target Model Catalog and Architecture Support Matrix](06_Models_Quantization_and_Inference/29_Target_Model_Catalog_and_Architecture_Support_Matrix/PROMPT.md)
- [30 — ROCmFPX Weight Formats and Quantization Recipes](06_Models_Quantization_and_Inference/30_ROCmFPX_Weight_Formats_and_Quantization_Recipes/PROMPT.md)
- [31 — Conversion, Imatrix, Calibration, and Quality Validation](06_Models_Quantization_and_Inference/31_Conversion_Imatrix_Calibration_and_Quality_Validation/PROMPT.md)
- [32 — llama.cpp Model Loading, Graph Construction, and Backend Lifecycle](06_Models_Quantization_and_Inference/32_llama_cpp_Model_Loading_Graph_Construction_and_Backend_Lifecycle/PROMPT.md)
- [33 — Attention Variants, KV Layouts, FlashAttention, and TurboQuant](06_Models_Quantization_and_Inference/33_Attention_Variants_KV_Layouts_FlashAttention_and_TurboQuant/PROMPT.md)
- [34 — MoE Routing, Expert Telemetry, and Expert Placement Inputs](06_Models_Quantization_and_Inference/34_MoE_Routing_Expert_Telemetry_and_Expert_Placement_Inputs/PROMPT.md)
- [35 — Recurrent, Mamba, SSM, Hybrid, and State-Shift Semantics](06_Models_Quantization_and_Inference/35_Recurrent_Mamba_SSM_Hybrid_and_State_Shift_Semantics/PROMPT.md)
- [36 — Native MTP, External Draft Models, and Speculative Decoding](06_Models_Quantization_and_Inference/36_Native_MTP_External_Draft_Models_and_Speculative_Decoding/PROMPT.md)
- [37 — gfx1151 HIP and Vulkan Kernel Optimization Opportunities](06_Models_Quantization_and_Inference/37_gfx1151_HIP_and_Vulkan_Kernel_Optimization_Opportunities/PROMPT.md)

### 07 — Distributed Runtime

- [38 — Distributed Runtime Goals, Cost Model, and Mode Selection](07_Distributed_Runtime/38_Distributed_Runtime_Goals_Cost_Model_and_Mode_Selection/PROMPT.md)
- [39 — Coordinator, Rank Worker, Session, and Persistent-Graph Architecture](07_Distributed_Runtime/39_Coordinator_Rank_Worker_Session_and_Persistent_Graph_Architecture/PROMPT.md)
- [40 — Full Replication, Request Routing, and Session Affinity](07_Distributed_Runtime/40_Full_Replication_Request_Routing_and_Session_Affinity/PROMPT.md)
- [41 — Remote Draft-Node Speculation](07_Distributed_Runtime/41_Remote_Draft_Node_Speculation/PROMPT.md)
- [42 — Two-Way Tensor Parallelism and Collective Placement](07_Distributed_Runtime/42_Two_Way_Tensor_Parallelism_and_Collective_Placement/PROMPT.md)
- [43 — Contiguous Layer Pipeline Parallelism and Microbatching](07_Distributed_Runtime/43_Contiguous_Layer_Pipeline_Parallelism_and_Microbatching/PROMPT.md)
- [44 — MoE-Aware Hybrid Distribution and Hot-Expert Replication](07_Distributed_Runtime/44_MoE_Aware_Hybrid_Distribution_and_Hot_Expert_Replication/PROMPT.md)
- [45 — Persistent Rank Protocol, Command Rings, and Graph Reuse](07_Distributed_Runtime/45_Persistent_Rank_Protocol_Command_Rings_and_Graph_Reuse/PROMPT.md)
- [46 — Scheduler, Continuous Batching, Backpressure, and Concurrency](07_Distributed_Runtime/46_Scheduler_Continuous_Batching_Backpressure_and_Concurrency/PROMPT.md)
- [47 — Topology Planner, Autotuner, and HIP-versus-Vulkan Selection](07_Distributed_Runtime/47_Topology_Planner_Autotuner_and_HIP_versus_Vulkan_Selection/PROMPT.md)
- [48 — Distributed Correctness, Determinism, Fault Recovery, and Degraded Mode](07_Distributed_Runtime/48_Distributed_Correctness_Determinism_Fault_Recovery_and_Degraded_Mode/PROMPT.md)

### 08 — Fabric and Transport

- [49 — Fabric Requirements and Transport Abstraction](08_Fabric_and_Transport/49_Fabric_Requirements_and_Transport_Abstraction/PROMPT.md)
- [50 — USB4STREAM and thunderbolt-net Implementation Options](08_Fabric_and_Transport/50_USB4STREAM_and_thunderbolt_net_Implementation_Options/PROMPT.md)
- [51 — Existing ggml RPC and ROCmFPX RDMA Transport Audit](08_Fabric_and_Transport/51_Existing_ggml_RPC_and_ROCmFPX_RDMA_Transport_Audit/PROMPT.md)
- [52 — Dual-Link Multipath: Striping, Alternation, Hedging, and Failover](08_Fabric_and_Transport/52_Dual_Link_Multipath_Striping_Alternation_Hedging_and_Failover/PROMPT.md)
- [53 — Message Framing, Credits, Flow Control, Integrity, and Security](08_Fabric_and_Transport/53_Message_Framing_Credits_Flow_Control_Integrity_and_Security/PROMPT.md)
- [54 — GPU-Visible Buffers, Coherence, Copies, and Zero-Copy Options](08_Fabric_and_Transport/54_GPU_Visible_Buffers_Coherence_Copies_and_Zero_Copy_Options/PROMPT.md)
- [55 — Fabric Microbenchmark Plan and USB4 Kernel-Patch Decision](08_Fabric_and_Transport/55_Fabric_Microbenchmark_Plan_and_USB4_Kernel_Patch_Decision/PROMPT.md)

### 09 — HaloKV Persistent Cache

- [56 — CachyLLama Cache Semantics and Porting Map](09_HaloKV_Persistent_Cache/56_CachyLLama_Cache_Semantics_and_Porting_Map/PROMPT.md)
- [57 — Compatibility Fingerprints, Versioning, and Topology Identity](09_HaloKV_Persistent_Cache/57_Compatibility_Fingerprints_Versioning_and_Topology_Identity/PROMPT.md)
- [58 — Rank-Local Ownership and Distributed Restore Coordination](09_HaloKV_Persistent_Cache/58_Rank_Local_Ownership_and_Distributed_Restore_Coordination/PROMPT.md)
- [59 — Immutable Pages, Segment Files, Indexes, and Prefix DAG](09_HaloKV_Persistent_Cache/59_Immutable_Pages_Segment_Files_Indexes_and_Prefix_DAG/PROMPT.md)
- [60 — System-Prompt Sharing, Deduplication, Copy-on-Write, and Continuations](09_HaloKV_Persistent_Cache/60_System_Prompt_Sharing_Deduplication_Copy_on_Write_and_Continuations/PROMPT.md)
- [61 — Attention KV, Recurrent, MTP, Speculative, Sampling, and RNG State](09_HaloKV_Persistent_Cache/61_Attention_KV_Recurrent_MTP_Speculative_Sampling_and_RNG_State/PROMPT.md)
- [62 — Async I/O, io_uring, Prefetch, DRAM Tiers, and GPU Mapping](09_HaloKV_Persistent_Cache/62_Async_I_O_io_uring_Prefetch_DRAM_Tiers_and_GPU_Mapping/PROMPT.md)
- [63 — Durability Modes, Atomic Commit, Crash Recovery, and Corruption Handling](09_HaloKV_Persistent_Cache/63_Durability_Modes_Atomic_Commit_Crash_Recovery_and_Corruption_Handling/PROMPT.md)
- [64 — Eviction, Garbage Collection, Quotas, User Isolation, and Privacy](09_HaloKV_Persistent_Cache/64_Eviction_Garbage_Collection_Quotas_User_Isolation_and_Privacy/PROMPT.md)
- [65 — Cache Inspection, Migration, Benchmarking, Write Amplification, and SSD Endurance](09_HaloKV_Persistent_Cache/65_Cache_Inspection_Migration_Benchmarking_Write_Amplification_and_SSD_Endurance/PROMPT.md)

### 10 — Product, Server, and Operations

- [66 — OpenAI-Compatible API, Server Semantics, and Error Model](10_Product_Server_and_Operations/66_OpenAI_Compatible_API_Server_Semantics_and_Error_Model/PROMPT.md)
- [67 — Configuration, Hardware Profiles, Model Manifests, and Plan Manifests](10_Product_Server_and_Operations/67_Configuration_Hardware_Profiles_Model_Manifests_and_Plan_Manifests/PROMPT.md)
- [68 — Model Lifecycle, Session Lifecycle, Admission Control, and Routing](10_Product_Server_and_Operations/68_Model_Lifecycle_Session_Lifecycle_Admission_Control_and_Routing/PROMPT.md)
- [69 — CLI, Admin API, Diagnostics, Health, Metrics, Logs, and Traces](10_Product_Server_and_Operations/69_CLI_Admin_API_Diagnostics_Health_Metrics_Logs_and_Traces/PROMPT.md)
- [70 — Packaging, systemd, Containers, Deployment, and Cold-Boot Procedure](10_Product_Server_and_Operations/70_Packaging_systemd_Containers_Deployment_and_Cold_Boot_Procedure/PROMPT.md)
- [71 — Security, Trust Boundaries, Permissions, Local Network, and Secrets](10_Product_Server_and_Operations/71_Security_Trust_Boundaries_Permissions_Local_Network_and_Secrets/PROMPT.md)
- [72 — Upgrades, Rollbacks, Protocol and Cache Migration, Backup, and Runbooks](10_Product_Server_and_Operations/72_Upgrades_Rollbacks_Protocol_and_Cache_Migration_Backup_and_Runbooks/PROMPT.md)

### 11 — Verification and Performance

- [73 — Benchmark Methodology, Terminology, Experimental Controls, and Data Schema](11_Verification_and_Performance/73_Benchmark_Methodology_Terminology_Experimental_Controls_and_Data_Schema/PROMPT.md)
- [74 — Single-Node HIP and Vulkan Baseline Matrix](11_Verification_and_Performance/74_Single_Node_HIP_and_Vulkan_Baseline_Matrix/PROMPT.md)
- [75 — Fabric Microbenchmarks and GPU-to-Peer-GPU End-to-End Tests](11_Verification_and_Performance/75_Fabric_Microbenchmarks_and_GPU_to_Peer_GPU_End_to_End_Tests/PROMPT.md)
- [76 — Distributed Mode Benchmark Matrix and Break-Even Analysis](11_Verification_and_Performance/76_Distributed_Mode_Benchmark_Matrix_and_Break_Even_Analysis/PROMPT.md)
- [77 — HaloKV Restore, Writeback, Hit-Rate, and Endurance Benchmarks](11_Verification_and_Performance/77_HaloKV_Restore_Writeback_Hit_Rate_and_Endurance_Benchmarks/PROMPT.md)
- [78 — Correctness, Regression, Determinism, and Model Quality Evaluation](11_Verification_and_Performance/78_Correctness_Regression_Determinism_and_Model_Quality_Evaluation/PROMPT.md)
- [79 — Stress, Soak, Long-Context, Multi-Session, Power, and Thermal Testing](11_Verification_and_Performance/79_Stress_Soak_Long_Context_Multi_Session_Power_and_Thermal_Testing/PROMPT.md)
- [80 — Fault Injection: Cable Pulls, Restarts, OOM, Disk-Full, and Corruption](11_Verification_and_Performance/80_Fault_Injection_Cable_Pulls_Restarts_OOM_Disk_Full_and_Corruption/PROMPT.md)
- [81 — CI Matrix, Release Gates, Reproducibility, and Performance Regression Policy](11_Verification_and_Performance/81_CI_Matrix_Release_Gates_Reproducibility_and_Performance_Regression_Policy/PROMPT.md)

### 12 — Project Execution and Governance

- [82 — Implementation Roadmap, Epics, Dependencies, and Exit Criteria](12_Project_Execution_and_Governance/82_Implementation_Roadmap_Epics_Dependencies_and_Exit_Criteria/PROMPT.md)
- [83 — Risk Register, Failure Modes, Mitigations, and Contingencies](12_Project_Execution_and_Governance/83_Risk_Register_Failure_Modes_Mitigations_and_Contingencies/PROMPT.md)
- [84 — On-Machine Research Plan, Experiment Cards, and Lab Notebook](12_Project_Execution_and_Governance/84_On_Machine_Research_Plan_Experiment_Cards_and_Lab_Notebook/PROMPT.md)
- [85 — Internet Research Backlog, Upstream Watch, and Knowledge Freshness](12_Project_Execution_and_Governance/85_Internet_Research_Backlog_Upstream_Watch_and_Knowledge_Freshness/PROMPT.md)
- [86 — Issues, Labels, Milestones, ADRs, Code Review, and Contribution Process](12_Project_Execution_and_Governance/86_Issues_Labels_Milestones_ADRs_Code_Review_and_Contribution_Process/PROMPT.md)

## Included support files

- `OUTPUT_STANDARD.md` — mandatory structure and evidence rules for every returned section.
- `PROJECT_CONTEXT.md` — compact project context for agents.
- `MERGE_INSTRUCTIONS.md` — safe procedure for assembling independently researched folders.
- `AGENTS.md.template` — operating rules for local Codex after the wiki is assembled.
- `section_index.json`, `.yaml`, and `.csv` — machine-readable section registry.
- `wiki_scaffold/` — empty target directory tree for the final wiki.
- `tools/validate_wiki.py` — checks the assembled wiki for required files.
- `tools/build_wiki_index.py` — regenerates a Markdown index from the section registry.
