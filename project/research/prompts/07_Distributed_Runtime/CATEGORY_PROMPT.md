# Category Research Agent Prompt — 07: Distributed Runtime

The project is a custom local LLM inference stack for two matched AMD Strix Halo systems connected by two tuned, low-latency USB4 host-to-host links. The intended source base is a fork of `charlie12345/ROCmFPX`, selectively incorporating the persistent SSD-backed KV-cache and agent-serving ideas from `fewtarius/CachyLLama` and `fewtarius/llama-ai`, while tracking `ggml-org/llama.cpp`. The product must choose among replication, remote speculative decoding, two-way tensor parallelism, pipeline parallelism, and MoE-aware hybrid execution, with a rank-local persistent cache and a specialized dual-link transport. The wiki will be consumed by local Codex and human engineers before and during implementation.

Build the complete wiki category **07: Distributed Runtime**. Research every numbered section below as a separate self-contained folder:

- `38_Distributed_Runtime_Goals_Cost_Model_and_Mode_Selection/` — 38: Distributed Runtime Goals, Cost Model, and Mode Selection
- `39_Coordinator_Rank_Worker_Session_and_Persistent_Graph_Architecture/` — 39: Coordinator, Rank Worker, Session, and Persistent-Graph Architecture
- `40_Full_Replication_Request_Routing_and_Session_Affinity/` — 40: Full Replication, Request Routing, and Session Affinity
- `41_Remote_Draft_Node_Speculation/` — 41: Remote Draft-Node Speculation
- `42_Two_Way_Tensor_Parallelism_and_Collective_Placement/` — 42: Two-Way Tensor Parallelism and Collective Placement
- `43_Contiguous_Layer_Pipeline_Parallelism_and_Microbatching/` — 43: Contiguous Layer Pipeline Parallelism and Microbatching
- `44_MoE_Aware_Hybrid_Distribution_and_Hot_Expert_Replication/` — 44: MoE-Aware Hybrid Distribution and Hot-Expert Replication
- `45_Persistent_Rank_Protocol_Command_Rings_and_Graph_Reuse/` — 45: Persistent Rank Protocol, Command Rings, and Graph Reuse
- `46_Scheduler_Continuous_Batching_Backpressure_and_Concurrency/` — 46: Scheduler, Continuous Batching, Backpressure, and Concurrency
- `47_Topology_Planner_Autotuner_and_HIP_versus_Vulkan_Selection/` — 47: Topology Planner, Autotuner, and HIP-versus-Vulkan Selection
- `48_Distributed_Correctness_Determinism_Fault_Recovery_and_Degraded_Mode/` — 48: Distributed Correctness, Determinism, Fault Recovery, and Degraded Mode

Apply `../OUTPUT_STANDARD.md` to every section. Use current primary sources, exact commits/versions/dates, and explicit claim labels. Separate Internet research from required on-machine investigation. Do not invent measurements or silently resolve conflicting evidence.

Return one downloadable folder named `07_Distributed_Runtime/` containing the numbered section folders at the exact paths listed above, plus a category `README.md` that summarizes the category, identifies authoritative pages, links all sections, and highlights cross-category dependencies.
